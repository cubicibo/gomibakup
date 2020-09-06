/*
 * wrappers.c
 *
 */


// Include required definitions first.
#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "wrappers.h"

#ifdef CHIBIOS
#include "ch.h"
#include "hal.h"
#include "gomibakup.h"
#endif	/* CHIBIOS */

/////////////////////////////////////
//Stm32 identifier
STATIC mp_obj_t wrapper_get_mcu_uuid(void) {
    #ifndef STM32_UUID
        #define STM32_UUID ((uint32_t *)0x1FFF7A10)
    #endif

    uint8_t ln = 3;
    mp_obj_t uuid[ln];

    uuid[0] = (mp_obj_t)mp_obj_new_int_from_uint(STM32_UUID[0]);
    uuid[1] = (mp_obj_t)mp_obj_new_int_from_uint(STM32_UUID[1]);
    uuid[2] = (mp_obj_t)mp_obj_new_int_from_uint(STM32_UUID[2]);

    // we return the period due to hard rules in those functions that could impose period < a
    return mp_obj_new_tuple(ln, uuid);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(wrapper_get_mcu_uuid_obj, wrapper_get_mcu_uuid);

///// Reset low level script kept in C side.
STATIC mp_obj_t wrapper_cpu_reset_script(void) {

    reset_script();
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(wrapper_cpu_reset_script_obj, wrapper_cpu_reset_script);

//Save cpu_write instruction in low level memory
STATIC mp_obj_t wrapper_cpu_write(mp_obj_t obj_addr, mp_obj_t obj_val) {

    if (mp_obj_is_int(obj_addr) && mp_obj_is_int(obj_val))
    	store_command(false, (uint16_t)mp_obj_get_int(obj_addr), (uint8_t)mp_obj_get_int(obj_val));

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(wrapper_cpu_write_obj, wrapper_cpu_write);

//Save cpu_read instruction in low level memory
STATIC mp_obj_t wrapper_cpu_read(mp_obj_t obj_addr, mp_obj_t obj_val) {

    if (mp_obj_is_int(obj_addr) && mp_obj_is_int(obj_val))
    	store_command(true, (uint16_t)mp_obj_get_int(obj_addr), (uint16_t)mp_obj_get_int(obj_val));

    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(wrapper_cpu_read_obj, wrapper_cpu_read);

//Execute script saved in low level memory (by default: NROM)
STATIC mp_obj_t wrapper_cpu_dump(void) {

   	request_dump();
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(wrapper_cpu_dump_obj, wrapper_cpu_dump);

//print a section of low level script kept in memory
STATIC mp_obj_t wrapper_cpu_get_script(void) {

	uint8_t size = 10;
	bool action[size];
    uint16_t addr[size];
    uint16_t val[size];

	fill_command(size, action, addr, val);
    
    uint8_t ln = 3;
    mp_obj_t ret_tuple[ln];
    ret_tuple[0] = mp_obj_new_bytearray(sizeof(action),action);
    //make arrays (tempReads, valid)
    ret_tuple[1] = mp_obj_new_bytearray(sizeof(addr),addr);
    ret_tuple[2] = mp_obj_new_bytearray(sizeof(val),val);

    return mp_obj_new_tuple(ln,ret_tuple); 
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(wrapper_cpu_get_script_obj, wrapper_cpu_get_script);

// Define all properties of the example module.
// Table entries are key/value pairs of the attribute name (a string)
// and the MicroPython object reference.
// All identifiers and strings are written as MP_QSTR_xxx and will be
// optimized to word-sized integers by the build system (interned strings).
STATIC const mp_rom_map_elem_t wrapper_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_wrappers) },
    /* BOARD SPECIFIC */
    { MP_ROM_QSTR(MP_QSTR_get_mcu_uuid), MP_ROM_PTR(&wrapper_get_mcu_uuid_obj) },

    { MP_ROM_QSTR(MP_QSTR_cpu_write), MP_ROM_PTR(&wrapper_cpu_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_cpu_read), MP_ROM_PTR(&wrapper_cpu_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_cpu_reset_script), MP_ROM_PTR(&wrapper_cpu_reset_script_obj) },
    { MP_ROM_QSTR(MP_QSTR_cpu_get_script), MP_ROM_PTR(&wrapper_cpu_get_script_obj) },
    { MP_ROM_QSTR(MP_QSTR_cpu_dump), MP_ROM_PTR(&wrapper_cpu_dump_obj) },
};
STATIC MP_DEFINE_CONST_DICT(wrapper_module_globals, wrapper_module_globals_table);

// Define module object.
const mp_obj_module_t wrapper_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&wrapper_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_wrappers, wrapper_user_cmodule, MODULE_WRAPPERS_ENABLED);
