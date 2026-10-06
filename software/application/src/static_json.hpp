#pragma once
#include "static_string.hpp"
#include <string.h>

// example:
//  {\n
//  0"temp_c":"23.1",\n
//  0"co2":"2222",\n
//  0"humidity":\n"1",\n
//  0"timestamp":"1791277820"}

// get (const char *field_name, void *dst) // you will have to know the type
// set (const char *field_name, void *dst) // you will have to know the type

template <size_t max_length>
class static_json {
public:
///////////////////////////////////////////////////////////////////////////////
    // extract different types of data
    bool get (const char *field, float *dst) // float
    {
        size_t field_len = strlen (field);
        // find the field
        for (size_t i = 0; i < container.size(); ++i)
        {
            const char *p = container.data () + i;
            // use strncmp and advance the offset to find the desired field.
        }

    }
    bool get (const char *field, int *dst) // int
    {
        size_t field_len = strlen (field);
        // find the field

    }
    bool get (const char *field, unsigned int *dst) // unsigned int
    {
        size_t field_len = strlen (field);
        // find the field

    }
    bool get (const char *field, char *dst, size_t length) // char * AKA string
    {
        size_t field_len = strlen (field);
        // find the field

    }
///////////////////////////////////////////////////////////////////////////////
    // set different types of data
    bool set (const char *field, float *dst) // float
    {
        size_t field_len = strlen (field);
        // find the field
        for 

    }
    bool set (const char *field, int *dst) // int
    {
        size_t field_len = strlen (field);
        // find the field

    }
    bool set (const char *field, unsigned int *dst) // unsigned int
    {
        size_t field_len = strlen (field);
        // find the field

    }
    bool set (const char *field, char *dst, size_t length) // char * AKA string
    {
        size_t field_len = strlen (field);
        // find the field

    }
///////////////////////////////////////////////////////////////////////////////
    bool clear () {
        return (container.clear ());
    }

    static_json (const char *buf)
    {
        size_t data_length = strlen (buf);
        if (data_length > container.capacity())
            data_length = container.capacity(); // truncate

        container.clear ();
        container.append (buf, data_length);
    }

    static_json (const static_string& str)
    {
        container.clear ();
        container = str; // operator= from static_string
    }

    // return 'true' if JSON was valid.
    bool process ()
    {
        // simplified JSON format.

        // rules:
        // - one flat object, no closures
        // - \escapes are not allowed
        // - arrays are not allowed
        // - duplicate fields are not allowed
        // - strings must be terminated(obviously)
        // - \r and/or \n are swapped for ' '(space)
        
        // {
        //     "temp":"23.1",
        //     "co2":"2222",
        //     "humidity":"1",
        //     "timestamp":"1760000000"
        // }

        bool has_start = false;
        bool has_end = false;

        bool in_string = false;
        // bool string_terminate = false;

        // as minimal as possible, sanity check for JSON, one pass.
        for (int c = 0; c < container.size(); c++)
        {
            char& p_c = container[c];
            switch (p_c) {
            case '\n':
            case '\r':
                p_c = ' ';
                continue;
            case '{': // track the closure
                has_start = true;
                continue;
            case '}': // track the closure
                has_end = true;
                break;
            case '"': // track the string closure
                in_string = !in_string;
            default:
                continue;
            }

            if (has_break)
                break; // break out of the loop
        }

        // no { , no } , or non-terminated string => definitely not valid JSON.
        if ( (!has_start || !has_end ) || in_string)
            return false;
        else // 'yup, my job is done here'.
            return true;
    }
private:
    sys::static_string <max_length> container;
};
