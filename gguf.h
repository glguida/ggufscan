#ifndef _GGUF_H_
#define _GGUF_H_

#include <stdint.h>
#include <stdbool.h>

#define GGUF_MAGIC_LE 0x46554747
#define GGUF_VERSION 3

typedef struct gguf_header {
	uint32_t magic;
	uint32_t version;
	uint64_t tensor_count;
	uint64_t metadata_kv_count;
} gguf_header_t;

typedef struct gguf_string {
	uint64_t len;
	char string[0];
} gguf_string_t;

enum gguf_metadata_value_type {
	GGUF_MVT_UINT8 = 0,
	GGUF_MVT_INT8 = 1,
	GGUF_MVT_UINT16 = 2,
	GGUF_MVT_INT16 = 3,
	GGUF_MVT_UINT32 = 4,
	GGUF_MVT_INT32 = 5,
	GGUF_MVT_FLOAT32 = 6,
	GGUF_MVT_BOOL = 7,
	GGUF_MVT_STRING = 8,
	GGUF_MVT_ARRAY = 9,
	GGUF_MVT_UINT64 = 10,
	GGUF_MVT_INT64 = 11,
	GGUF_MVT_FLOAT64 = 12,
};

typedef uint32_t gguf_metadata_value_type_t;

typedef struct gguf_array {
	gguf_metadata_value_type_t type;
	uint64_t len;
	char array[0]; /* gguf_metadata_value_t */
} gguf_array_t;

typedef union gguf_metadata_value {
	uint8_t uint8;
	int8_t int8;
	uint16_t uint16;
	int16_t int16;
	uint32_t uint32;
	int32_t int32;
	float float32;
	uint64_t uint64;
	int64_t int64;
	double float64;
	bool bool_;
	gguf_string_t string;
	gguf_array_t array;
} gguf_metadata_value_t;

typedef struct gguf_metadata_kv_t {
	gguf_string_t key;
	gguf_metadata_value_type_t value_type;
	gguf_metadata_value_t value;
} gguf_metadata_kv_t;

#endif

/*
Local Variables:
    mode: c
    c-file-style: "linux"
    tab-width: 8
    indent-tabs-mode: t
    c-basic-offset: 8
End:
*/
