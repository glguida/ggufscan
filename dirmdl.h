#ifndef _DIRMDL_H_
#define _DIRMDL_H_

#define DEFINE_CREATE_KEY_FUNC(TYPE, FORMAT, SUFFIX)	\
  int create_key_value_##SUFFIX(char *key, uint64_t keylength, TYPE value)

DEFINE_CREATE_KEY_FUNC(uint8_t, "%u", uint8);
DEFINE_CREATE_KEY_FUNC(int8_t,  "%d", int8);
DEFINE_CREATE_KEY_FUNC(uint16_t, "%u", uint16);
DEFINE_CREATE_KEY_FUNC(int16_t,  "%d", int16);
DEFINE_CREATE_KEY_FUNC(uint32_t, "%u", uint32);
DEFINE_CREATE_KEY_FUNC(int32_t,  "%d", int32);
DEFINE_CREATE_KEY_FUNC(uint64_t, "%llu", uint64);
DEFINE_CREATE_KEY_FUNC(int64_t,  "%lld", int64);
DEFINE_CREATE_KEY_FUNC(float,    "%f", float32);
DEFINE_CREATE_KEY_FUNC(double,   "%f", float64);
DEFINE_CREATE_KEY_FUNC(bool,   "%d", bool);

#undef DEFINE_CREATE_KEY_FUNC

int create_key_value_string(char *key, uint64_t keylength, char *str, uint64_t strlength);

void create_array_key(char *key, uint64_t keylen);
void end_array(void);

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
