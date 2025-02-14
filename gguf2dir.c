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
GGUF_SCAN_FUNC(bool, bool);

void *
gguf_scan_string(void *ptr, uint64_t *len, char **string)
{
	ptr = gguf_scan_uint64(ptr, len);
	*string = ptr;
	return ptr + *len;
}

void *
create_array(void *ptr, char *key, uint64_t keylen)
{
	int r;
	char * dir;
	uint32_t type;
	uint64_t len;

	if (asprintf((char ** restrict)&dir, "%.*s", (int)keylen, key) != 0) {
		perror("asprintf");
		exit(EXIT_FAILURE);
	}

	ptr = gguf_scan_uint32(ptr, &type);
	ptr = gguf_scan_uint64(ptr, &len);

	if (mkdir(dir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(dir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	switch (type)
	{
		

	}
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
		ptr = gguf_scan_uint8(ptr, &v.int8);
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
	for (uint64_t i = 0; i < n_meta; i++)
		ptr = gguf_scan_metadata_keyvalue(ptr);
	return ptr;
}

int main(int argc, char *argv[])
{
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

	void *ptr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		exit(EXIT_FAILURE);
	}

	ptr = gguf_scan_header(ptr);

	if (mkdir(output_dir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(output_dir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	ptr = gguf_scan_metadata(ptr);

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
