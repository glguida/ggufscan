#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "gguf.h"
#include "dirmdl.h"

uint64_t n_tens;
uint64_t n_meta;

/*
  TODO: 'general.alignment' in the metadata key-value may specify the
  alignment for tensor data.
*/
uint64_t align = 32;

void *
gguf_scan_header(void *ptr)
{
	gguf_header_t *h = (gguf_header_t *)ptr;
	if (h->magic != GGUF_MAGIC_LE) {
		fprintf(stderr, "Not a GGUF file\n");
		exit(EXIT_FAILURE);
	}

	if (h->version != GGUF_VERSION) {
		fprintf(stderr, "Version '%d' not supported\n", h->version);
		exit(EXIT_FAILURE);
	}

	n_tens = h->tensor_count;
	n_meta = h->metadata_kv_count;

	return (void *)(h + 1);
}

#define GGUF_SCAN_FUNC(TYPE, SUFFIX)		\
	static void *				\
	gguf_scan_##SUFFIX (void *ptr, TYPE *v)	\
	{					\
		TYPE *vptr = (TYPE *)ptr;	\
		*v = *vptr;			\
		return (void *)(vptr + 1);	\
	}

GGUF_SCAN_FUNC(int8_t, int8);
GGUF_SCAN_FUNC(uint8_t, uint8);
GGUF_SCAN_FUNC(int16_t, int16);
GGUF_SCAN_FUNC(uint16_t, uint16);
GGUF_SCAN_FUNC(int32_t, int32);
GGUF_SCAN_FUNC(uint32_t, uint32);
GGUF_SCAN_FUNC(int64_t, int64);
GGUF_SCAN_FUNC(uint64_t, uint64);
GGUF_SCAN_FUNC(float, float32);
GGUF_SCAN_FUNC(double, float64);
GGUF_SCAN_FUNC(uint8_t, bool);

void *
gguf_scan_string(void *ptr, uint64_t *len, char **string)
{
	ptr = gguf_scan_uint64(ptr, len);
	*string = ptr;
	return ptr + *len;
}

void *
gguf_scan_metadata_value(void *ptr, char *key, uint64_t keylen, uint32_t type)
{
	gguf_metadata_value_t v;

	switch (type) {
	case GGUF_MVT_UINT8:
		ptr = gguf_scan_uint8(ptr, &v.uint8);
		create_key_value_uint8(key, keylen, v.uint8);
		break;
	case GGUF_MVT_INT8:
		ptr = gguf_scan_int8(ptr, &v.int8);
		create_key_value_int8(key, keylen, v.int8);
		break;
	case GGUF_MVT_UINT16:
		ptr = gguf_scan_uint16(ptr, &v.uint16);
		create_key_value_uint16(key, keylen, v.uint16);
		break;
	case GGUF_MVT_INT16:
		ptr = gguf_scan_int16(ptr, &v.int16);
		create_key_value_int16(key, keylen, v.int16);
		break;
	case GGUF_MVT_UINT32:
		ptr = gguf_scan_uint32(ptr, &v.uint32);
		create_key_value_uint32(key, keylen, v.uint32);
		break;
	  case GGUF_MVT_INT32:
		  ptr = gguf_scan_int32(ptr, &v.int32);
		  create_key_value_int16(key, keylen, v.uint16);
		  break;
	case GGUF_MVT_UINT64:
		ptr = gguf_scan_uint64(ptr, &v.uint64);
		create_key_value_uint64(key, keylen, v.uint64);
		break;
	case GGUF_MVT_INT64:
		ptr = gguf_scan_int64(ptr, &v.int64);
		create_key_value_int64(key, keylen, v.int64);
		break;
	case GGUF_MVT_FLOAT32:
		ptr = gguf_scan_float32(ptr, &v.float32);
		create_key_value_float32(key, keylen, v.float32);
		break;
	case GGUF_MVT_FLOAT64:
		ptr = gguf_scan_float64(ptr, &v.float64);
		create_key_value_float64(key, keylen, v.float64);
		break;
	case GGUF_MVT_BOOL:
		ptr = gguf_scan_bool(ptr, &v.bool_);
		create_key_value_bool(key, keylen, v.bool_);
		break;
	case GGUF_MVT_STRING: {
		uint64_t len;
		char *str;
		ptr = gguf_scan_string(ptr, &len, &str);
		create_key_value_string(key, keylen, str, len);
		break;
	}
	case GGUF_MVT_ARRAY: {
		uint32_t type;
		uint64_t len;
		ptr = gguf_scan_uint32(ptr, &type);
		ptr = gguf_scan_uint64(ptr, &len);
		create_array_key(key, keylen);
		for (uint64_t i = 0; i < len; i++) {
			char *key;
			asprintf((char ** restrict)&key, "%"PRId64, i);
			ptr = gguf_scan_metadata_value(ptr, key, strlen(key), type);
			free(key);
		}
		end_array();
		break;
	}
	default:
		fprintf(stderr, "Invalid Metadata Type value %d\n", type);
		exit(EXIT_FAILURE);
	}

	return ptr;
}

void *
gguf_scan_metadata_keyvalue(void *ptr)
{
	char *key;
	uint64_t keylen;
	uint32_t type;

	ptr = gguf_scan_string(ptr, &keylen, &key);
	ptr = gguf_scan_uint32(ptr, &type);
	ptr = gguf_scan_metadata_value(ptr, key, keylen, type);

	return ptr;
}

void *
gguf_scan_metadata(void *ptr)
{
	start_metadata();

	for (uint64_t i = 0; i < n_meta; i++)
		ptr = gguf_scan_metadata_keyvalue(ptr);


	end_metadata();

	return ptr;
}

void *
gguf_scan_one_tensor(void *ptr, void *tensor_data)
{
	char *name;
	uint64_t namelen;
	uint32_t ndims;
	uint64_t *dims;
	uint64_t offset;
	uint32_t ggmltype;

	ptr = gguf_scan_string(ptr, &namelen, &name);
	ptr = gguf_scan_uint32(ptr, &ndims);
	dims = malloc(ndims * sizeof(uint64_t));
	if (dims == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for (uint32_t i = 0; i < ndims; i++)
		ptr = gguf_scan_uint64(ptr, dims + i);

	ptr = gguf_scan_uint32(ptr, &ggmltype);
	ptr = gguf_scan_uint64(ptr, &offset);

	if (tensor_data) {
		create_tensor(name, namelen, dims, ndims, ggmltype, tensor_data + offset);
	}
	return ptr;
}

void *
gguf_scan_tensors(void *ptr, void *ptr_start)
{
	/*
	  We have to scan tensor twice.

	  The first time to find the starting offset of tensor data,
	  the second time to actually create tensors.
	*/
	void *ptr_tensor_info = ptr;
	for (uint64_t i = 0; i < n_tens; i++) {
		ptr = gguf_scan_one_tensor(ptr, NULL);
	}
	void *ptr_tensor_data = ptr_start + align_offset(ptr - ptr_start, align);

	start_tensors();

	ptr = ptr_tensor_info;
	for (uint64_t i = 0; i < n_tens; i++) {
		ptr = gguf_scan_one_tensor(ptr, ptr_tensor_data);
	}

	end_tensors();

	return ptr;
}

int main(int argc, char *argv[])
{
	void *ptr_start, *ptr;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <GGUF model> <output directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *filename = argv[1];
	const char *output_dir = argv[2];
	
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		perror("fstat");
		close(fd);
		exit(EXIT_FAILURE);
	}

	if (sb.st_size == 0) {
		fprintf(stderr, "Error: File is empty\n");
		close(fd);
		exit(EXIT_FAILURE);
	}

	ptr_start = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr_start == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(EXIT_FAILURE);
	}

	ptr = ptr_start;
	ptr = gguf_scan_header(ptr);

	if (mkdir(output_dir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(output_dir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	printf("Scanning metadata...");
	fflush(stdout);
	ptr = gguf_scan_metadata(ptr);
	printf("done.\n");

	printf("Scanning tensors...");
	fflush(stdout);
	ptr = gguf_scan_tensors(ptr, ptr_start);
	printf("done.\n");

	munmap(ptr, sb.st_size);
	close(fd);
	return EXIT_SUCCESS;
}

/*
Local Variables:
    mode: c
    c-file-style: "linux"
    tab-width: 8
    indent-tabs-mode: t
    c-basic-offset: 8
End:
*/
