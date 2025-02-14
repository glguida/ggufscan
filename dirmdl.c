#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

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

int create_array_key(const char *key, uint64_t keylen)
{
	FILE *fd;

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
}



#define CREATE_KEY_FUNC(TYPE, FORMAT, SUFFIX)				\
	int create_key_value_##SUFFIX(char *key, uint64_t keylen, TYPE value) \
	{								\
		if (create_key(key, keylen) != 0)			\
			return -1;					\
		if (add_data(FORMAT, value) != 0)			\
			return -1;					\
		end_key();						\
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
