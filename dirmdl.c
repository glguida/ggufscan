#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "gguf.h"
#include "dirmdl.h"

static FILE *curkey = NULL;

int create_key(const char *key, uint64_t keylen)
{
	if (curkey != NULL) {
		fclose(curkey);
		curkey = NULL;
	}
	
	char *filename = malloc(keylen + 1);
	if (!filename) {
		perror("malloc");
		return -1;
	}
	memcpy(filename, key, keylen);
	filename[keylen] = '\0';

	curkey = fopen(filename, "w");
	free(filename);
	if (!curkey) {
		perror("fopen");
		return -1;
	}

	return 0;
}

int add_data(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	int ret = vfprintf(curkey, format, args);
	va_end(args);
	if (ret < 0) {
		perror("vfprintf");
		fclose(curkey);
		return -1;
	}

	return 0;
}

void end_key(void)
{
	fclose(curkey);
	curkey = NULL;
}

int create_array_key(char *key, uint64_t keylen)
{

	if (curkey != NULL) {
		fclose(curkey);
		curkey = NULL;
	}
	
	char *keydir = malloc(keylen + 1);
	if (!keydir) {
		perror("malloc");
		return -1;
	}
	memcpy(keydir, key, keylen);
	keydir[keylen] = '\0';

	if (mkdir(keydir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(keydir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	free(keydir);
	return 0;
}

int end_array(void)
{
	if (chdir("..") != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int start_metadata(void)
{
	char *metadir = "metadata";

	if (mkdir(metadir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(metadir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	return 0;
}

int end_metadata(void)
{
	if (chdir("..") != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}
	return 0;
}

#define CREATE_KEY_FUNC(TYPE, FORMAT, SUFFIX)				\
	int create_key_value_##SUFFIX(char *key, uint64_t keylen, TYPE value) \
	{								\
		if (create_key(key, keylen) != 0)			\
			return -1;					\
		if (add_data(FORMAT, value) != 0)			\
			return -1;					\
		end_key();						\
		return 0;						\
	}

CREATE_KEY_FUNC(uint8_t, "%u\n", uint8);
CREATE_KEY_FUNC(int8_t,  "%d\n", int8);
CREATE_KEY_FUNC(uint16_t, "%u\n", uint16);
CREATE_KEY_FUNC(int16_t,  "%d\n", int16);
CREATE_KEY_FUNC(uint32_t, "%u\n", uint32);
CREATE_KEY_FUNC(int32_t,  "%d\n", int32);
CREATE_KEY_FUNC(uint64_t, "%llu\n", uint64);
CREATE_KEY_FUNC(int64_t,  "%lld\n", int64);
CREATE_KEY_FUNC(float,    "%f\n", float32);
CREATE_KEY_FUNC(double,   "%f\n", float64);
CREATE_KEY_FUNC(bool,   "%d\n", bool);

#undef CREATE_KEY_FUNC

int create_key_value_string(char *key, uint64_t keylen, char *str, uint64_t strlength)
{
	if (create_key(key, keylen) != 0)
		return -1;
	if (add_data("%.*s\n", (int)strlength, str) != 0)
		return -1;
	end_key();
	return 0;
}

int start_tensors(void)
{
	char *tensdir = "tensors";

	if (mkdir(tensdir, 0755) != 0) {
		perror("mkdir");
		exit(EXIT_FAILURE);
	}

	if (chdir(tensdir) != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}

	return 0;
}

int end_tensors(void)
{
	if (chdir("..") != 0) {
		perror("chdir");
		exit(EXIT_FAILURE);
	}
	return 0;
}

const char *
ggmltype_to_string(uint32_t ggmltype)
{
	switch (ggmltype)
	{
	case GGML_TYPE_F32:
		return "F32";
	case GGML_TYPE_F16:
		return "F16";
	case GGML_TYPE_Q4_0:
		return "Q4_0";
	case GGML_TYPE_Q4_1:
		return "Q4_1";
	case GGML_TYPE_Q5_0:
		return "Q5_0";
	case GGML_TYPE_Q5_1:
		return "Q5_1";
	case GGML_TYPE_Q8_0:
		return "Q8_0";
	case GGML_TYPE_Q8_1:
		return "Q8_1";
	case GGML_TYPE_Q2_K:
		return "Q2_K";
	case GGML_TYPE_Q3_K:
		return "Q3_K";
	case GGML_TYPE_Q4_K:
		return "Q4_K";
	case GGML_TYPE_Q5_K:
		return "Q5_K";
	case GGML_TYPE_Q6_K:
		return "Q6_K";
	case GGML_TYPE_Q8_K:
		return "Q8_K";
	case GGML_TYPE_IQ2_XXS:
		return "IQ2_XXS";
	case GGML_TYPE_IQ2_XS:
		return "IQ2_XS";
	case GGML_TYPE_IQ3_XXS:
		return "IQ3_XXS";
	case GGML_TYPE_IQ1_S:
		return "IQ1_S";
	case GGML_TYPE_IQ4_NL:
		return "IQ4_NL";
	case GGML_TYPE_IQ3_S:
		return "IQ3_S";
	case GGML_TYPE_IQ2_S:
		return "IQ2_S";
	case GGML_TYPE_IQ4_XS:
		return "IQ4_XS";
	case GGML_TYPE_I8:
		return "I8";
	case GGML_TYPE_I16:
		return "I16";
	case GGML_TYPE_I32:
		return "I32";
	case GGML_TYPE_I64:
		return "I64";
	case GGML_TYPE_F64:
		return "F64";
	case GGML_TYPE_IQ1_M:
		return "IQ1_M";
	default:
		return "Unknown";
	}
}

size_t
ggmltype_to_size(uint32_t ggmltype)
{
	/* TODO: Support quantised sizes. */
	switch (ggmltype)
	{
	case GGML_TYPE_I8:
		return 1;
	case GGML_TYPE_I16:
	case GGML_TYPE_F16:
		return 2;
	case GGML_TYPE_F32:
	case GGML_TYPE_I32:
		return 4;
	case GGML_TYPE_I64:
	case GGML_TYPE_F64:
		return 8;
	default:
		fprintf(stderr,
			"Uknown size for type '%s'\n",
			ggmltype_to_string(ggmltype));
		exit(EXIT_FAILURE);
	}
}

int create_tensor(char *name, uint64_t namelen,
		  uint64_t *dims, uint64_t ndims,
		  uint32_t ggmltype, void *data)
{
	uint64_t sz = ggmltype_to_size(ggmltype);

	printf("Adding tensor %.*s %p\n", (int)namelen, name, data);

	create_array_key(name, namelen);

	create_key("dims", 4);
	for (uint64_t i = 0; i < ndims; i++) {
		printf("sz: %"PRId64" * %"PRId64"\n", sz, dims[i]);
		sz *= dims[i];
		add_data("%"PRId64, dims[i]);
		if (i < ndims - 1)
			add_data(" ");
		else
			add_data("\n");
	}
	end_key();

	create_key("datatype", 8);
	add_data("%s\n", ggmltype_to_string(ggmltype));

	int fd = open("data", O_RDWR | O_CREAT, 0666);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	if (ftruncate(fd, sz) == -1) {
		perror("ftruncate");
		close(fd);
		return 1;
	}

	void *map = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	memcpy(map, data, sz);

	if (munmap(map, sz) == -1) {
		perror("munmap");
	}
	close(fd);

	end_key();

	end_array();

	return 0;
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
