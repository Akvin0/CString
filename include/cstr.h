#pragma once

/**
 * @file cstr.h
 * @brief Thread-safe dynamic string implementation for C.
 */

#ifndef CSTR_H
#define CSTR_H

#include <Windows.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @def CSTR_INVALID
     * @brief Error indicator for invalid positions/sizes
     */
    static const size_t invalid = (size_t)-1;

    /**
     * @struct CString
     * @brief Thread-safe dynamic string container
     *
     * @var data     - Pointer to null-terminated character buffer
     * @var length   - Current string length (excluding null-terminator)
     * @var capacity - Total allocated buffer size
     * @var cs       - Critical section for thread synchronization
     */
    typedef struct
    {
        char* data;           ///< Character buffer
        size_t length;        ///< Current string length
        size_t capacity;      ///< Allocated buffer size
        CRITICAL_SECTION cs;  ///< Thread synchronization primitive
    }CString;

    /**
     * @brief Duplicate null-terminated C string
     * @param str Source string to copy
     * @return New allocated copy on success, NULL on failure
     * @note Safe replacement for non-standard strdup()
     * @warning Caller must free result with free()
     */
    char* cstr_strdup(_In_ const char* str)
    {
        size_t len = strlen(str) + 1;
        char* buf = (char*)malloc(len);
        if (buf)
            memcpy(buf, str, len);
        return buf;
    }

    /**
     * @brief Duplicate null-terminated wide string
     * @param str Source wide string to copy
     * @return New allocated copy on success, NULL on failure
     * @note Wide char version of cstr_strdup()
     * @warning Caller must free result with free()
     */
    wchar_t* cstr_wcsdup(_In_ const wchar_t* str)
    {
        size_t len = (wcslen(str) + 1) * sizeof(wchar_t);
        wchar_t* buf = (wchar_t*)malloc(len);
        if (buf)
            memcpy(buf, str, len);
        return buf;
    }

    /**
     * @brief Initialize a new empty CString
     * @param obj Pointer to CString object to initialize
     * @return true on success, false on allocation failure
     * @note Creates empty string with capacity 1
     */
    bool cstr_create(_Inout_ CString* obj)
    {
        if (!obj)
            return false;

        char* data = (char*)malloc(1);
        if (data == NULL)
            return false;

        data[0] = '\0';

        obj->data = data;
        obj->length = 0;
        obj->capacity = 1;

        InitializeCriticalSection(&obj->cs);

        return true;
    }

    /**
     * @brief Create CString copy from another CString
     * @param obj  Destination CString
     * @param obj2 Source CString
     * @return true on success, false on allocation failure
     */
    bool cstr_create_from_cstr(_Inout_ CString* obj, _In_ CString* obj2)
    {
        if (!obj || !obj2)
            return false;

        obj->data = cstr_strdup(obj2->data);
        obj->length = obj2->length;
        obj->capacity = obj2->capacity;

        InitializeCriticalSection(&obj->cs);

        return true;
    }

    /**
     * @brief Create CString from null-terminated C string
     * @param obj  Destination CString
     * @param data Source C string
     * @return true on success, false on allocation failure
     */
    bool cstr_create_from_chars(_Inout_ CString* obj, _In_ const char* data)
    {
        if (!obj || !data)
            return false;

        obj->data = cstr_strdup(data);
        obj->length = strlen(data);
        obj->capacity = obj->length + 1;

        InitializeCriticalSection(&obj->cs);

        return true;
    }

    /**
     * @brief Create CString from wide character string
     * @param obj  Destination CString
     * @param data Source wide string
     * @return true on success, false on conversion/allocation failure
     * @note Uses WideCharToMultiByte with ANSI code page
     */
    bool cstr_create_from_wchars(_Inout_ CString* obj, _In_ const wchar_t* data)
    {
        if (!obj || !data)
            return false;

        int len = WideCharToMultiByte(CP_ACP, 0, data, -1, NULL, 0, NULL, NULL);
        if (len == 0)
            return false;

        char* mb_data = (char*)malloc(len);
        if (!mb_data)
            return false;

        if (!WideCharToMultiByte(CP_ACP, 0, data, -1, mb_data, len, NULL, NULL))
        {
            free(mb_data);
            return false;
        }

        obj->data = mb_data;
        obj->length = strlen(mb_data);
        obj->capacity = len;

        InitializeCriticalSection(&obj->cs);

        return true;
    }

    /**
     * @brief Create CString from binary buffer
     * @param obj    Destination CString
     * @param buffer Source binary data
     * @param size   Number of bytes to copy
     * @return true on success, false on allocation failure
     * @note Adds null-terminator after buffer contents
     */
    bool cstr_create_from_buffer(_Inout_ CString* obj, _In_ uint8_t* buffer, _In_ size_t size)
    {
        if (!obj || !buffer)
            return false;

        char* data = (char*)malloc(size + 1);
        if (data == NULL)
            return false;

        memcpy(data, (void*)buffer, size);
        data[size] = '\0';

        obj->data = data;
        obj->length = size;
        obj->capacity = size + 1;

        InitializeCriticalSection(&obj->cs);

        return true;
    }

    /**
     * @brief Destroy CString and release resources
     * @param obj CString to destroy
     * @return true on success, false for invalid object
     * @note Securely erases memory before freeing
     */
    bool cstr_destroy(_In_ CString* obj)
    {
        if (!obj)
            return false;

        if (obj->data)
        {
            SecureZeroMemory(obj->data, obj->capacity);
            free(obj->data);
            obj->data = NULL;
        }

        DeleteCriticalSection(&obj->cs);

        obj->length = 0;
        obj->capacity = 0;

        return true;
    }

    /**
     * @brief Acquire exclusive access
     * @param obj CString object
     */
    void cstr_lock(_In_ CString* obj)
    {
        if (obj)
            EnterCriticalSection(&obj->cs);
    }

    /**
     * @brief Release exclusive access
     * @param obj CString object
     */
    void cstr_unlock(_In_ CString* obj)
    {
        if (obj)
            LeaveCriticalSection(&obj->cs);
    }

    /**
     * @brief Get character at specific index
     * @param obj   CString object
     * @param index Character position (0-based)
     * @param chr   Output character
     * @return true if index valid, false otherwise
     * @note Thread-safe version with bounds checking
     */
    boolean cstr_at(_In_ CString* obj, _In_ size_t index, _Inout_ char* chr)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (index >= obj->length)
        {
            cstr_unlock(obj);
            return false;
        }

        *chr = obj->data[index];

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Direct character access (unsynchronized)
     * @param obj   CString object
     * @param index Character position
     * @return Character or 0 for invalid index
     * @warning Not thread-safe - use between lock/unlock calls
     */
    char cstr_get(_In_ CString* obj, _In_ size_t index)
    {
        if (!obj)
            return 0;

        cstr_lock(obj);

        char out = obj->data[index];

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Get first character
     * @param obj CString object
     * @return First character or 0 if empty
     */
    char cstr_front(_In_ CString* obj)
    {
        if (!obj)
            return 0;

        cstr_lock(obj);

        char out = cstr_get(obj, 0);

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Get last character
     * @param obj CString object
     * @return Last character or 0 if empty
     */
    char cstr_back(_In_ CString* obj)
    {
        if (!obj)
            return 0;

        cstr_lock(obj);

        char out = cstr_get(obj, obj->length - 1);

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Get raw character buffer
     * @param obj CString object
     * @return Pointer to internal buffer
     * @warning Buffer valid until next modifying operation
     */
    char* cstr_data(_In_ CString* obj)
    {
        if (!obj)
            return 0;

        cstr_lock(obj);

        char* out = obj->data;

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Get current string length
     * @param obj CString object
     * @return Length in bytes or CSTR_INVALID
     */
    size_t cstr_length(_In_ CString* obj)
    {
        if (!obj)
            return invalid;

        cstr_lock(obj);

        size_t out = obj->length;

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Get allocated buffer capacity
     * @param obj CString object
     * @return Capacity in bytes or CSTR_INVALID
     */
    size_t cstr_capacity(_In_ CString* obj)
    {
        if (!obj)
            return invalid;

        cstr_lock(obj);

        size_t out = obj->capacity;

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Check if string is empty
     * @param obj CString object
     * @return true if empty, false otherwise
     */
    bool cstr_empty(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        bool out = obj->data == NULL || obj->length == 0;

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Resize internal buffer
     * @param obj  CString object
     * @param size New buffer size
     * @return true on success, false on allocation failure
     * @note Does not modify string contents
     */
    bool cstr_resize(_In_ CString* obj, _In_ size_t size)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        char* new_data = (char*)realloc(obj->data, size);
        if (new_data == NULL)
        {
            cstr_unlock(obj);
            return false;
        }

        obj->data = new_data;
        obj->capacity = size;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Minimize buffer to fit current contents
     * @param obj CString object
     * @return true on success, false on allocation failure
     */
    bool cstr_shrink_to_fit(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (!cstr_resize(obj, obj->length + 1))
        {
            cstr_unlock(obj);
            return false;
        }

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Clear string contents
     * @param obj CString object
     * @return true on success
     * @note Securely erases buffer and resets length
     */
    bool cstr_clear(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        SecureZeroMemory(obj->data, obj->capacity);
        obj->length = 0;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Append single ASCII character
     * @param obj CString object
     * @param chr Character to append
     * @return true on success
     */
    bool cstr_push_back_char(_In_ CString* obj, _In_ char chr)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (obj->length + 1 >= obj->capacity)
        {
            if (!cstr_resize(obj, obj->length + 2))
            {
                cstr_unlock(obj);
                return false;
            }
        }

        obj->data[obj->length] = chr;
        obj->data[obj->length + 1] = '\0';
        obj->length++;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Append wide character
     * @param obj CString object
     * @param chr Wide character to append
     * @return true on success
     * @note Converts to multibyte using system code page
     */
    bool cstr_push_back_wchar(_In_ CString* obj, _In_ wchar_t chr)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        wchar_t wstr[2] = { chr, L'\0' };
        int required_mb_len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (required_mb_len <= 0)
        {
            cstr_unlock(obj);
            return false;
        }

        char* mb_str = (char*)malloc(required_mb_len);
        if (!mb_str)
        {
            cstr_unlock(obj);
            return false;
        }

        if (WideCharToMultiByte(CP_ACP, 0, wstr, -1, mb_str, required_mb_len, NULL, NULL) == 0)
        {
            free(mb_str);
            cstr_unlock(obj);
            return false;
        }

        size_t data_len = required_mb_len - 1;

        size_t new_length = obj->length + data_len;
        size_t required_capacity = new_length + 1;

        if (required_capacity > obj->capacity)
        {
            size_t new_capacity = required_capacity;
            char* new_data = (char*)realloc(obj->data, new_capacity);
            if (!new_data)
            {
                free(mb_str);
                cstr_unlock(obj);
                return false;
            }
            obj->data = new_data;
            obj->capacity = new_capacity;
        }

        memcpy(obj->data + obj->length, mb_str, data_len);
        obj->length = new_length;
        obj->data[new_length] = '\0';

        free(mb_str);
        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Remove last character
     * @param obj CString object
     * @return true if character removed, false if empty
     */
    bool cstr_pop_back(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (obj->length == 0)
        {
            cstr_unlock(obj);
            return false;
        }

        obj->data[obj->length - 1] = 0;
        obj->length--;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Append CString contents
     * @param obj  Destination CString
     * @param obj2 Source CString
     * @return true on success
     */
    bool cstr_append_cstr(_In_ CString* obj, _In_ CString* obj2)
    {
        if (!obj || !obj2)
            return false;

        cstr_lock(obj);

        size_t new_length = obj->length + obj2->length;
        size_t required_capacity = new_length + 1;

        if (required_capacity > obj->capacity)
        {
            if (!cstr_resize(obj, required_capacity))
            {
                cstr_unlock(obj);
                return false;
            }
        }

        memcpy(obj->data + obj->length, obj2->data, obj2->length);
        obj->data[new_length] = '\0';
        obj->length = new_length;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Append C string
     * @param obj  Destination CString
     * @param data Null-terminated source string
     * @return true on success
     */
    bool cstr_append_chars(_In_ CString* obj, _In_ const char* data)
    {
        if (!obj || !data)
            return false;

        cstr_lock(obj);

        size_t data_len = strlen(data);
        size_t new_length = obj->length + data_len;
        size_t required_capacity = new_length + 1;

        if (required_capacity > obj->capacity)
        {
            if (!cstr_resize(obj, required_capacity))
            {
                cstr_unlock(obj);
                return false;
            }
        }

        memcpy(obj->data + obj->length, data, data_len);
        obj->data[new_length] = '\0';
        obj->length = new_length;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Append wide string
     * @param obj  Destination CString
     * @param data Null-terminated wide string
     * @return true on success
     * @note Converts using system code page
     */
    bool cstr_append_wchars(_In_ CString* obj, _In_ const wchar_t* data)
    {
        if (!obj || !data)
            return false;

        cstr_lock(obj);

        int len = WideCharToMultiByte(CP_ACP, 0, data, -1, NULL, 0, NULL, NULL);
        if (len == 0)
        {
            cstr_unlock(obj);
            return false;
        }

        char* mb_data = (char*)malloc(len);
        if (!mb_data)
        {
            cstr_unlock(obj);
            return false;
        }

        if (WideCharToMultiByte(CP_ACP, 0, data, -1, mb_data, len, NULL, NULL) == 0)
        {
            free(mb_data);
            cstr_unlock(obj);
            return false;
        }

        size_t data_len = strlen(mb_data);
        size_t new_length = obj->length + data_len;
        size_t required_capacity = new_length + 1;

        if (required_capacity > obj->capacity)
        {
            if (!cstr_resize(obj, required_capacity))
            {
                free(mb_data);
                cstr_unlock(obj);
                return false;
            }
        }

        memcpy(obj->data + obj->length, mb_data, data_len);
        obj->data[new_length] = '\0';
        obj->length = new_length;

        free(mb_data);

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Extract substring
     * @param obj    Source CString
     * @param dest   Destination CString
     * @param start  Starting index
     * @param length Number of characters to extract
     * @return true on success
     * @note Automatically clamps to valid range
     */
    bool cstr_substring(_In_ CString* obj, _Inout_ CString* dest, _In_ size_t start, _In_ size_t length)
    {
        if (!obj || !dest)
            return false;

        cstr_lock(obj);

        if (start >= obj->length)
        {
            cstr_unlock(obj);
            return false;
        }

        size_t max_length = obj->length - start;
        if (length > max_length)
            length = max_length;

        char* buffer = (char*)malloc(length + 1);
        if (!buffer)
        {
            cstr_unlock(obj);
            return false;
        }

        memcpy(buffer, obj->data + start, length);
        buffer[length] = '\0';

        if (!cstr_create_from_chars(dest, buffer))
        {
            free(buffer);
            cstr_unlock(obj);
            return false;
        }

        free(buffer);

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Remove characters
     * @param obj   CString object
     * @param index Starting position
     * @param size  Number of characters to remove
     * @return true on success
     */
    bool cstr_erase(_In_ CString* obj, _In_ size_t index, _In_ size_t size)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (index >= obj->length || size == 0)
        {
            cstr_unlock(obj);
            return false;
        }

        if (size > obj->length - index)
            size = obj->length - index;

        size_t new_length = obj->length - size;
        size_t move_size = (obj->length - (index + size)) + 1;

        memmove(obj->data + index, obj->data + index + size, move_size);
        obj->length = new_length;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Insert character
     * @param obj   CString object
     * @param index Insertion position
     * @param chr   Character to insert
     * @return true on success
     */
    bool cstr_insert(_In_ CString* obj, _In_ size_t index, _In_ char chr)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (index > obj->length)
        {
            cstr_unlock(obj);
            return false;
        }

        size_t new_length = obj->length + 1;
        size_t required_capacity = new_length + 1;

        if (required_capacity > obj->capacity)
        {
            if (!cstr_resize(obj, required_capacity))
            {
                cstr_unlock(obj);
                return false;
            }
        }


        memmove(obj->data + index + 1, obj->data + index, (obj->length - index) + 1);
        obj->data[index] = chr;
        obj->length = new_length;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Swap contents between two CStrings
     * @param obj  First CString
     * @param obj2 Second CString
     * @return true on success
     */
    bool cstr_swap(_In_ CString* obj, _In_ CString* obj2)
    {
        if (!obj || !obj2)
            return false;

        cstr_lock(obj);
        cstr_lock(obj2);

        char* temp_data = obj->data;
        obj->data = obj2->data;
        obj2->data = temp_data;

        size_t temp_length = obj->length;
        obj->length = obj2->length;
        obj2->length = temp_length;

        size_t temp_capacity = obj->capacity;
        obj->capacity = obj2->capacity;
        obj2->capacity = temp_capacity;

        cstr_unlock(obj2);
        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Find substring (CString)
     * @param obj  CString to search
     * @param obj2 Substring to find
     * @return Starting index or CSTR_INVALID
     */
    size_t cstr_find_cstr(_In_ CString* obj, _In_ CString* obj2)
    {
        if (!obj || !obj2)
            return invalid;

        cstr_lock(obj);
        cstr_lock(obj2);

        char* pos = strstr(obj->data, obj2->data);
        size_t out = (pos != NULL) ? (size_t)(pos - obj->data) : invalid;

        cstr_unlock(obj2);
        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Find substring (C string)
     * @param obj  CString to search
     * @param data Null-terminated substring
     * @return Starting index or CSTR_INVALID
     */
    size_t cstr_find_chars(_In_ CString* obj, _In_ const char* data)
    {
        if (!obj || !data)
            return invalid;

        cstr_lock(obj);

        char* pos = strstr(obj->data, data);
        size_t out = (pos != NULL) ? (size_t)(pos - obj->data) : invalid;

        cstr_unlock(obj);

        return out;
    }

    /**
     * @brief Find substring (wide string)
     * @param obj  CString to search
     * @param data Null-terminated wide substring
     * @return Starting index or CSTR_INVALID
     * @note Converts using system code page
     */
    size_t cstr_find_wchars(_In_ CString* obj, _In_ const wchar_t* data)
    {
        if (!obj || !data)
            return invalid;

        int len = WideCharToMultiByte(CP_ACP, 0, data, -1, NULL, 0, NULL, NULL);
        if (len == 0)
            return invalid;

        char* mb_data = (char*)malloc(len);
        if (!mb_data)
            return invalid;

        if (WideCharToMultiByte(CP_ACP, 0, data, -1, mb_data, len, NULL, NULL) == 0)
        {
            free(mb_data);
            return invalid;
        }

        cstr_lock(obj);

        char* pos = strstr(obj->data, mb_data);
        size_t result = (pos != NULL) ? (size_t)(pos - obj->data) : invalid;

        cstr_unlock(obj);

        free(mb_data);

        return result;
    }

    /**
     * @brief Convert to uppercase
     * @param obj CString object
     * @return true on success
     */
    bool cstr_to_upper(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        for (size_t i = 0; i < obj->length; ++i)
            obj->data[i] = (char)toupper((unsigned char)obj->data[i]);

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Convert to lowercase
     * @param obj CString object
     * @return true on success
     */
    bool cstr_to_lower(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        for (size_t i = 0; i < obj->length; ++i)
            obj->data[i] = (char)tolower((unsigned char)obj->data[i]);

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Trim whitespace from both ends
     * @param obj CString object
     * @return true if modified, false otherwise
     */
    bool cstr_trim(_In_ CString* obj)
    {
        if (!obj)
            return false;

        cstr_lock(obj);

        if (obj->length == 0)
        {
            cstr_unlock(obj);
            return false;
        }

        size_t start = 0;
        size_t end = obj->length - 1;

        while (start <= end && isspace((unsigned char)obj->data[start]))
            start++;

        while (end >= start && isspace((unsigned char)obj->data[end]))
            end--;

        size_t new_length = (start <= end) ? (end - start + 1) : 0;

        if (start > 0)
            memmove(obj->data, obj->data + start, new_length);

        obj->data[new_length] = '\0';
        obj->length = new_length;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Extract token using delimiters
     * @param obj        Source CString
     * @param token      Output token
     * @param delimiters Separator characters
     * @param start_pos  Starting/ending position (updated)
     * @return true if token found
     */
    bool cstr_tokenize(_In_ CString* obj, _Inout_ CString* token, _In_ const char* delimiters, _Inout_ size_t* start_pos)
    {
        if (!obj || !delimiters || !start_pos || *start_pos >= obj->length)
            return false;

        cstr_lock(obj);

        size_t len = obj->length;
        size_t pos = *start_pos;
        
        while (pos < len && strchr(delimiters, obj->data[pos]) != NULL)
            pos++;

        if (pos >= len)
        {
            *start_pos = pos;
            cstr_unlock(obj);
            return false;
        }

        size_t token_start = pos;

        while (pos < len && strchr(delimiters, obj->data[pos]) == NULL)
            pos++;

        size_t token_end = pos;

        size_t token_len = token_end - token_start;
        char* temp = (char*)malloc(token_len + 1);
        if (!temp)
        {
            cstr_unlock(obj);
            return false;
        }

        memcpy(temp, obj->data + token_start, token_len);
        temp[token_len] = '\0';

        if (!cstr_create_from_chars(token, temp))
        {
            free(temp);
            cstr_unlock(obj);
            return false;
        }

        free(temp);

        *start_pos = (token_end < len) ? token_end + 1 : len;

        cstr_unlock(obj);

        return true;
    }

    /**
     * @brief Advanced tokenization with zones/escaping
     * @param obj          Source CString
     * @param token        Output token
     * @param delimiters   Separator characters
     * @param zone_pairs   Zone delimiter pairs (e.g., "\"\"''")
     * @param escape_chars Escape characters
     * @param start_pos    Starting/ending position (updated)
     * @return true if token found
     *
     * @code
     * size_t pos = 0;
     * CString str, token;
     * cstr_create_from_chars(&str, "Hello, \"my world\"!");
     * while (cstr_tokenize_ex(&str, &token, " ", "\"\"", "\\", &pos))
     *     printf("Token: %s\n", cstr_data(&token));
     * @endcode
     */
    bool cstr_tokenize_ex(_In_ CString* obj, _Inout_ CString* token, _In_ const char* delimiters, _In_ const char* zone_pairs, _In_ const char* escape_chars, _Inout_ size_t* start_pos)
    {
        if (!obj || !delimiters || !start_pos || *start_pos >= obj->length)
            return false;

        cstr_lock(obj);

        size_t len = obj->length;
        size_t pos = *start_pos;

        while (pos < len && strchr(delimiters, obj->data[pos]) != NULL)
            pos++;

        if (pos >= len)
        {
            *start_pos = pos;
            cstr_unlock(obj);
            return false;
        }

        size_t token_start = pos;
        size_t token_end = invalid;
        bool in_zone = false;
        char zone_end = '\0';
        bool escape = false;

        for (; pos < len; pos++)
        {
            char c = obj->data[pos];

            if (escape)
            {
                escape = false;
                continue;
            }

            if (in_zone)
            {
                if (c == zone_end)
                {
                    in_zone = false;
                    zone_end = '\0';
                }
            }
            else
            {
                if (strchr(delimiters, c) != NULL)
                {
                    token_end = pos;
                    break;
                }

                if (zone_pairs)
                {
                    for (int z = 0; zone_pairs[z] != '\0'; z += 2)
                    {
                        if (zone_pairs[z + 1] == '\0')
                            break;
                        if (c == zone_pairs[z])
                        {
                            in_zone = true;
                            zone_end = zone_pairs[z + 1];
                            break;
                        }
                    }
                }

                if (escape_chars && strchr(escape_chars, c) != NULL)
                    escape = true;
            }
        }

        token_end = (pos == len) ? len : token_end;

        size_t token_len = token_end - token_start;
        char* temp = (char*)malloc(token_len + 1);
        if (!temp)
        {
            cstr_unlock(obj);
            return false;
        }

        memcpy(temp, obj->data + token_start, token_len);
        temp[token_len] = '\0';

        if (!cstr_create_from_chars(token, temp))
        {
            free(temp);
            cstr_unlock(obj);
            return false;
        }

        free(temp);
        *start_pos = (token_end < len) ? token_end + 1 : len;

        cstr_unlock(obj);

        return true;
    }

#ifdef __cplusplus
}
#endif

#endif // CSTR_H
