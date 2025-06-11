#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>

// Constants for buffer sizes
#define MAX_OUTPUT_SIZE 8192
#define TEMP_BUFFER_SIZE 2048

// Helper function to reverse a string
static void reverse_string(char *str, int length) {
    for (int i = 0, j = length - 1; i < j; i++, j--) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

// Convert an integer to a string in the specified base
static int integer_to_string(uintmax_t value, char *buffer, int base, int use_uppercase, 
    int is_signed, int is_negative, int min_digits) {
    // Mark unused parameter to suppress the warning
    (void)is_signed;

const char *digits = use_uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
int position = 0;

// Rest of the function remains the same...

    // Handle zero with zero precision
    if (value == 0 && min_digits == 0) {
        buffer[position] = '\0';
        return 0;
    }

    // Convert number to digits
    if (value == 0) {
        buffer[position++] = '0';
    } else {
        while (value) {
            buffer[position++] = digits[value % base];
            value /= base;
        }
    }

    // Add leading zeros if needed
    while (position < min_digits) {
        buffer[position++] = '0';
    }

    // Add negative sign if needed
    if (is_negative) {
        buffer[position++] = '-';
    }

    buffer[position] = '\0';
    reverse_string(buffer, position);
    return position;
}

// Convert a floating-point number to a string
static int float_to_string(double value, char *buffer, int precision, int format_flags, 
                         int use_uppercase) {
    int length = 0;
    int is_negative = value < 0;

    // Handle special cases: NaN and Infinity
    if (isnan(value)) {
        strcpy(buffer, use_uppercase ? "NAN" : "nan");
        return 3;
    }
    if (isinf(value)) {
        strcpy(buffer, is_negative ? (use_uppercase ? "-INF" : "-inf") : 
                                     (use_uppercase ? "INF" : "inf"));
        return is_negative ? 4 : 3;
    }

    // Handle sign flags
    if (is_negative) {
        value = -value;
    } else if (format_flags & 0x01) { // '+' flag
        *buffer++ = '+';
        length++;
    } else if (format_flags & 0x02) { // ' ' flag
        *buffer++ = ' ';
        length++;
    }

    if (is_negative) {
        *buffer++ = '-';
        length++;
    }

    // Default precision is 6 if not specified
    if (precision < 0) {
        precision = 6;
    }

    // Round the number and split into integer and fractional parts
    double rounded = round(value * pow(10.0, precision)) / pow(10.0, precision);
    uintmax_t integer_part = (uintmax_t)rounded;
    double fractional_part = rounded - integer_part;

    // Convert integer part
    length += integer_to_string(integer_part, buffer, 10, 0, 0, 0, 0);
    buffer += strlen(buffer);

    // Add decimal point and fractional part if needed
    if (precision > 0 || (format_flags & 0x10)) { // '#' flag ensures decimal point
        *buffer++ = '.';
        length++;

        for (int i = 0; i < precision; i++) {
            fractional_part *= 10.0;
            int digit = (int)fractional_part;
            *buffer++ = '0' + digit;
            fractional_part -= digit;
            length++;
        }
    }

    *buffer = '\0';
    return length;
}

// Convert a number to scientific notation
static int scientific_to_string(double value, char *buffer, int precision, 
                              int use_uppercase, int format_flags) {
    int length = 0;
    int is_negative = value < 0;
    int exponent = 0;

    // Handle special cases
    if (isnan(value)) {
        strcpy(buffer, use_uppercase ? "NAN" : "nan");
        return 3;
    }
    if (isinf(value)) {
        strcpy(buffer, is_negative ? (use_uppercase ? "-INF" : "-inf") : 
                                     (use_uppercase ? "INF" : "inf"));
        return is_negative ? 4 : 3;
    }

    // Handle signs
    if (is_negative) {
        value = -value;
    } else if (format_flags & 0x01) {
        *buffer++ = '+';
        length++;
    } else if (format_flags & 0x02) {
        *buffer++ = ' ';
        length++;
    }

    if (is_negative) {
        *buffer++ = '-';
        length++;
    }

    // Handle zero
    if (value == 0.0) {
        *buffer++ = '0';
        length++;
        if (precision > 0 || (format_flags & 0x10)) {
            *buffer++ = '.';
            length++;
            for (int i = 0; i < precision; i++) {
                *buffer++ = '0';
                length++;
            }
        }
        *buffer++ = use_uppercase ? 'E' : 'e';
        *buffer++ = '+';
        *buffer++ = '0';
        *buffer++ = '0';
        length += 4;
        *buffer = '\0';
        return length;
    }

    // Normalize to range [1, 10)
    while (value >= 10.0) {
        value /= 10.0;
        exponent++;
    }
    while (value < 1.0) {
        value *= 10.0;
        exponent--;
    }

    // Round and split
    if (precision < 0) {
        precision = 6;
    }
    double rounded = round(value * pow(10.0, precision)) / pow(10.0, precision);
    uintmax_t integer_part = (uintmax_t)rounded;
    double fractional_part = rounded - integer_part;

    // Convert mantissa
    *buffer++ = '0' + integer_part;
    length++;

    if (precision > 0 || (format_flags & 0x10)) {
        *buffer++ = '.';
        length++;
        for (int i = 0; i < precision; i++) {
            fractional_part *= 10.0;
            int digit = (int)fractional_part;
            *buffer++ = '0' + digit;
            fractional_part -= digit;
            length++;
        }
    }

    // Add exponent
    *buffer++ = use_uppercase ? 'E' : 'e';
    length++;
    *buffer++ = (exponent >= 0) ? '+' : '-';
    length++;
    if (exponent < 0) exponent = -exponent;
    if (exponent < 10) {
        *buffer++ = '0';
        length++;
    }
    length += integer_to_string(exponent, buffer, 10, 0, 0, 0, 0);
    return length;
}

// Convert a number to hexadecimal floating-point format
static int hex_float_to_string(double value, char *buffer, int precision, 
                             int use_uppercase, int format_flags) {
    int length = 0;
    int is_negative = value < 0;
    int exponent = 0;

    // Handle special cases
    if (isnan(value)) {
        strcpy(buffer, use_uppercase ? "NAN" : "nan");
        return 3;
    }
    if (isinf(value)) {
        strcpy(buffer, is_negative ? (use_uppercase ? "-INF" : "-inf") : 
                                     (use_uppercase ? "INF" : "inf"));
        return is_negative ? 4 : 3;
    }

    if (value == 0.0) {
        strcpy(buffer, use_uppercase ? "0X0.0P+0" : "0x0.0p+0");
        return 7;
    }

    // Handle signs
    if (is_negative) {
        *buffer++ = '-';
        length++;
        value = -value;
    } else if (format_flags & 0x01) {
        *buffer++ = '+';
        length++;
    } else if (format_flags & 0x02) {
        *buffer++ = ' ';
        length++;
    }

    // Normalize to [1, 2)
    while (value >= 2.0) {
        value /= 2.0;
        exponent++;
    }
    while (value < 1.0) {
        value *= 2.0;
        exponent--;
    }

    if (precision < 0) {
        precision = 6;
    }

    // Round and split
    double rounded = round(value * pow(16.0, precision)) / pow(16.0, precision);
    uintmax_t integer_part = (uintmax_t)rounded;
    double fractional_part = rounded - integer_part;

    // Convert hexadecimal format
    *buffer++ = '0';
    *buffer++ = use_uppercase ? 'X' : 'x';
    *buffer++ = '1';
    length += 3;

    if (precision > 0 || (format_flags & 0x10)) {
        *buffer++ = '.';
        length++;
        for (int i = 0; i < precision; i++) {
            fractional_part *= 16.0;
            int digit = (int)fractional_part;
            *buffer++ = digit < 10 ? '0' + digit : 
                                     (use_uppercase ? 'A' + digit - 10 : 'a' + digit - 10);
            fractional_part -= digit;
            length++;
        }
    }

    // Add exponent
    *buffer++ = use_uppercase ? 'P' : 'p';
    length++;
    *buffer++ = (exponent >= 0) ? '+' : '-';
    length++;
    if (exponent < 0) exponent = -exponent;
    length += integer_to_string(exponent, buffer, 10, 0, 0, 0, 0);
    return length;
}

// Choose shortest representation between float and scientific notation
static int shortest_float_to_string(double value, char *buffer, int precision, 
                                  int use_uppercase, int format_flags) {
    char scientific_buffer[512], float_buffer[512];
    int scientific_length, float_length;

    if (precision < 0) {
        precision = 6;
    }
    if (precision == 0) {
        precision = 1; // %g requires at least one significant digit
    }

    // Get both representations
    scientific_length = scientific_to_string(value, scientific_buffer, precision - 1, 
                                           use_uppercase, format_flags);
    float_length = float_to_string(value, float_buffer, precision - 1, format_flags, 
                                  use_uppercase);

    // Remove trailing zeros unless '#' flag is set
    if (!(format_flags & 0x10)) {
        char *ptr = scientific_buffer;
        if (strchr(scientific_buffer, '.')) {
            while (*ptr) ptr++;
            ptr--;
            while (*ptr == '0') ptr--;
            if (*ptr == '.') ptr--;
            ptr[1] = '\0';
            scientific_length = ptr - scientific_buffer + 1;
        }
        ptr = float_buffer;
        if (strchr(float_buffer, '.')) {
            while (*ptr) ptr++;
            ptr--;
            while (*ptr == '0') ptr--;
            if (*ptr == '.') ptr--;
            ptr[1] = '\0';
            float_length = ptr - float_buffer + 1;
        }
    }

    // Choose shorter representation
    if (scientific_length <= float_length) {
        strcpy(buffer, scientific_buffer);
        return scientific_length;
    } else {
        strcpy(buffer, float_buffer);
        return float_length;
    }
}

// Convert a pointer to a string
static int pointer_to_string(void *pointer, char *buffer) {
    *buffer++ = '0';
    *buffer++ = 'x';
    return 2 + integer_to_string((uintptr_t)pointer, buffer, 16, 0, 0, 0, 0);
}

// Main formatting function
int my_vsnprintf(char *output, size_t max_size, const char *format, va_list arguments) {
    char *current_output = output;
    size_t total_chars_written = 0;
    size_t actual_chars_written = 0;

    if (max_size == 0) return 0;
    if (max_size == 1) {
        *output = '\0';
        return 0;
    }

    while (*format) {
        if (*format != '%') {
            if (actual_chars_written < max_size - 1) {
                *current_output++ = *format;
                actual_chars_written++;
            }
            total_chars_written++;
            format++;
            continue;
        }

        format++; // Skip '%'

        // Parse flags: '+' (0x01), ' ' (0x02), '-' (0x04), '0' (0x08), '#' (0x10)
        int format_flags = 0;
        while (1) {
            if (*format == '+') format_flags |= 0x01;
            else if (*format == ' ') format_flags |= 0x02;
            else if (*format == '-') format_flags |= 0x04;
            else if (*format == '0') format_flags |= 0x08;
            else if (*format == '#') format_flags |= 0x10;
            else break;
            format++;
        }

        // Parse width
        int width = 0;
        if (*format == '*') {
            width = va_arg(arguments, int);
            format++;
        } else {
            while (isdigit(*format)) {
                width = width * 10 + (*format++ - '0');
            }
        }

        // Parse precision
        int precision = -1;
        if (*format == '.') {
            format++;
            precision = 0;
            if (*format == '*') {
                precision = va_arg(arguments, int);
                format++;
            } else {
                while (isdigit(*format)) {
                    precision = precision * 10 + (*format++ - '0');
                }
            }
        }

        // Parse length modifiers
        enum { NONE, HH, H, L, LL, J, Z, T, LONG_DOUBLE } length_modifier = NONE;
        if (*format == 'h') {
            format++;
            length_modifier = (*format == 'h') ? (format++, HH) : H;
        } else if (*format == 'l') {
            format++;
            length_modifier = (*format == 'l') ? (format++, LL) : L;
        } else if (*format == 'j') {
            length_modifier = J;
            format++;
        } else if (*format == 'z') {
            length_modifier = Z;
            format++;
        } else if (*format == 't') {
            length_modifier = T;
            format++;
        } else if (*format == 'L') {
            length_modifier = LONG_DOUBLE;
            format++;
        }

        char specifier = *format++;
        char temp_buffer[TEMP_BUFFER_SIZE] = {0};
        int temp_length = 0;

        switch (specifier) {
            case 'd':
            case 'i': {
                intmax_t value;
                switch (length_modifier) {
                    case HH: value = (char)va_arg(arguments, int); break;
                    case H: value = (short)va_arg(arguments, int); break;
                    case L: value = va_arg(arguments, long); break;
                    case LL: value = va_arg(arguments, long long); break;
                    case J: value = va_arg(arguments, intmax_t); break;
                    case Z: value = va_arg(arguments, size_t); break;
                    case T: value = va_arg(arguments, ptrdiff_t); break;
                    default: value = va_arg(arguments, int); break;
                }
                int is_negative = value < 0;
                uintmax_t abs_value = is_negative ? -value : value;

                if (!is_negative) {
                    if (format_flags & 0x01) temp_buffer[temp_length++] = '+';
                    else if (format_flags & 0x02) temp_buffer[temp_length++] = ' ';
                }
                temp_length += integer_to_string(abs_value, temp_buffer + temp_length, 
                                               10, 0, 1, is_negative, precision);
                break;
            }
            case 'u':
            case 'x':
            case 'X':
            case 'o': {
                uintmax_t value;
                int base = (specifier == 'u') ? 10 : (specifier == 'o') ? 8 : 16;
                int use_uppercase = (specifier == 'X');

                switch (length_modifier) {
                    case HH: value = (unsigned char)va_arg(arguments, unsigned int); break;
                    case H: value = (unsigned short)va_arg(arguments, unsigned int); break;
                    case L: value = va_arg(arguments, unsigned long); break;
                    case LL: value = va_arg(arguments, unsigned long long); break;
                    case J: value = va_arg(arguments, uintmax_t); break;
                    case Z: value = va_arg(arguments, size_t); break;
                    case T: value = va_arg(arguments, ptrdiff_t); break;
                    default: value = va_arg(arguments, unsigned int); break;
                }

                if ((format_flags & 0x10) && value != 0) {
                    if (specifier == 'o') {
                        temp_buffer[temp_length++] = '0';
                    } else if (specifier == 'x' || specifier == 'X') {
                        temp_buffer[temp_length++] = '0';
                        temp_buffer[temp_length++] = use_uppercase ? 'X' : 'x';
                    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
                }
                temp_length += integer_to_string(value, temp_buffer + temp_length, 
                                               base, use_uppercase, 0, 0, precision);
                break;
            }
            case 'c': {
                temp_buffer[temp_length++] = (char)va_arg(arguments, int);
                break;
            }
            case 's': {
                const char *string = va_arg(arguments, const char *);
                if (!string) string = "(null)";

                int string_length = strlen(string);
                if (precision >= 0 && precision < string_length) {
                    string_length = precision;
                }
                strncpy(temp_buffer, string, string_length);
                temp_length = string_length;
                break;
            }
            case 'p': {
                temp_length = pointer_to_string(va_arg(arguments, void *), temp_buffer);
                break;
            }
            case 'n': {
                int *count_ptr = va_arg(arguments, int *);
                *count_ptr = total_chars_written;
                break;
            }
            case 'f':
            case 'F': {
                double value = (length_modifier == LONG_DOUBLE) ? 
                              va_arg(arguments, long double) : va_arg(arguments, double);
                temp_length = float_to_string(value, temp_buffer, precision, format_flags, 
                                             (specifier == 'F'));
                break;
            }
            case 'e':
            case 'E': {
                double value = (length_modifier == LONG_DOUBLE) ? 
                              va_arg(arguments, long double) : va_arg(arguments, double);
                temp_length = scientific_to_string(value, temp_buffer, precision, 
                                                 (specifier == 'E'), format_flags);
                break;
            }
            case 'g':
            case 'G': {
                double value = (length_modifier == LONG_DOUBLE) ? 
                              va_arg(arguments, long double) : va_arg(arguments, double);
                temp_length = shortest_float_to_string(value, temp_buffer, precision, 
                                                     (specifier == 'G'), format_flags);
                break;
            }
            case 'a':
            case 'A': {
                double value = (length_modifier == LONG_DOUBLE) ? 
                              va_arg(arguments, long double) : va_arg(arguments, double);
                temp_length = hex_float_to_string(value, temp_buffer, precision, 
                                                (specifier == 'A'), format_flags);
                break;
            }
            case '%': {
                temp_buffer[temp_length++] = '%';
                break;
            }
            default: {
                temp_buffer[temp_length++] = '%';
                temp_buffer[temp_length++] = specifier;
            }
        }

        // Handle padding
        int padding_length = (width > temp_length) ? width - temp_length : 0;

        if (format_flags & 0x04) { // Left justify
            for (int i = 0; i < temp_length && actual_chars_written < max_size - 1; i++) {
                *current_output++ = temp_buffer[i];
                actual_chars_written++;
            }
            total_chars_written += temp_length;

            for (int i = 0; i < padding_length; i++) {
                if (actual_chars_written < max_size - 1) {
                    *current_output++ = ' ';
                    actual_chars_written++;
                }
                total_chars_written++;
            }
        } else { // Right justify
            char padding_char = (format_flags & 0x08 && !(specifier == 's' || specifier == 'c')) ? 
                               '0' : ' ';
            for (int i = 0; i < padding_length; i++) {
                if (actual_chars_written < max_size - 1) {
                    *current_output++ = padding_char;
                    actual_chars_written++;
                }
                total_chars_written++;
            }
            for (int i = 0; i < temp_length && actual_chars_written < max_size - 1; i++) {
                *current_output++ = temp_buffer[i];
                actual_chars_written++;
            }
            total_chars_written += temp_length;
        }
    }

    if (actual_chars_written < max_size) {
        *current_output = '\0';
    } else {
        output[max_size - 1] = '\0';
    }

    return total_chars_written;
}

// Wrapper for snprintf
int my_snprintf(char *output, size_t max_size, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int result = my_vsnprintf(output, max_size, format, arguments);
    va_end(arguments);
    return result;
}

// Wrapper for printf
int my_printf(const char *format, ...) {
    char output[MAX_OUTPUT_SIZE];
    va_list arguments;
    va_start(arguments, format);
    int count = my_vsnprintf(output, MAX_OUTPUT_SIZE, format, arguments);
    va_end(arguments);

    if (fputs(output, stdout) < 0) {
        errno = EIO;
        return -1;
    }

    return count;
}

// Main function with test cases
int main() {
    // Basic formatting
    my_printf("Integer: %d\n", 42);
    my_printf("Negative Integer: %d\n", -42);
    my_printf("Unsigned: %u\n", 3000000000U);
    my_printf("Hex lower: %x\n", 255);
    my_printf("Hex upper: %X\n", 255);
    my_printf("Octal: %o\n", 255);
    my_printf("Char: %c\n", 'A');
    my_printf("String: %s\n", "Hello, World!");
    my_printf("Null string: %s\n", (char *)NULL);

    // Floating point
    my_printf("Float default: %f\n", 3.14159);
    my_printf("Float with precision: %.2f\n", 3.14159);
    my_printf("Scientific notation: %e\n", 3.14159);
    my_printf("Scientific notation upper: %E\n", 3.14159);
    my_printf("Auto format: %g\n", 3.14159);
    my_printf("Auto format large: %g\n", 3141590000.0);
    my_printf("Auto format small: %.2g\n", 0.000123);

    // Hex floating point
    my_printf("Hex float lower: %a\n", 3.14159);
    my_printf("Hex float upper: %A\n", 3.14159);

    // Special floating-point values
    my_printf("Infinity: %f\n", INFINITY);
    my_printf("Negative Infinity: %f\n", -INFINITY);
    my_printf("NaN: %f\n", NAN);
    my_printf("Infinity (scientific): %e\n", INFINITY);
    my_printf("NaN (hex): %a\n", NAN);

    // Width and justification
    my_printf("Width padded int: %5d\n", 42);
    my_printf("Left justified: %-5d!\n", 42);
    my_printf("Zero padded: %05d\n", 42);

    // Flags
    my_printf("Plus sign: %+d\n", 42);
    my_printf("Space sign: % d\n", 42);
    my_printf("Hex with # flag: %#x\n", 255);
    my_printf("Octal with # flag: %#o\n", 255);
    my_printf("Float with # flag: %#f\n", 1.0);
    my_printf("Scientific with # flag: %#e\n", 1.0);

    // Precision for integers
    my_printf("Precision int: %.5d\n", 42);
    my_printf("Zero with precision 0: %.0d\n", 0);
    my_printf("Width and precision: %8.5d\n", 42);

    // Special cases
    my_printf("Percent sign: %%\n");
    my_printf("Pointer: %p\n", (void *)0x12345678);

    // Length modifiers
    my_printf("Long: %ld\n", 2147483648L);
    my_printf("Long long: %lld\n", 9223372036854775807LL);
    my_printf("Short: %hd\n", (short)32767);

    // %n specifier
    int count;
    my_printf("Characters so far: %n%d\n", &count, count);

    // Truncation
    char small_buffer[10];
    int written = my_snprintf(small_buffer, sizeof(small_buffer), 
                            "This is a long string that will be truncated");
    my_printf("Truncated string: '%s', would have written %d chars\n", 
             small_buffer, written);

    // Negative width and precision
    my_printf("Negative width: %*d\n", -5, 42);
    my_printf("Negative precision: %.5f\n", 3.14159);

    // Large numbers
    my_printf("Large integer: %d\n", INTMAX_MAX);
    my_printf("Large float: %f\n", 1e308);

    // Test asterisk width/precision and length modifiers
    my_printf("Asterisk width/precision: %*.*lld\n", 10, 5, 123LL);
    my_printf("Size_t: %zu\n", (size_t)4294967295);
    my_printf("Null pointer: %p\n", (void *)NULL);
    my_printf("Empty string: %s\n", "");
    my_printf("Truncated string: %.3s\n", "abcdef");
    my_printf("Extreme width: %100d\n", 42);
    my_printf("Combined flags: %+0#10.5x\n", 255);

    return 0;
}