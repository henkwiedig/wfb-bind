#!/usr/bin/python3
import sys

def convert_to_c_array(input_file, array_name):
    try:
        # Read binary data from the input file
        with open(input_file, "rb") as f:
            data = f.read()
        
        # Generate the C array as a formatted string
        c_array = f"const unsigned char {array_name}[] = {{\n"
        for i, byte in enumerate(data):
            # Format bytes into hex and group by 12 bytes per line
            c_array += f"0x{byte:02X}, "
            if (i + 1) % 12 == 0:  # Newline after 12 bytes
                c_array += "\n"
        c_array = c_array.rstrip(", \n")  # Remove trailing comma and newline
        c_array += "\n};\n"
        c_array += f"const size_t {array_name}_size = {len(data)};\n"

        # Print the generated C array
        print(c_array)
        print(f"Successfully created C array for '{array_name}' with {len(data)} bytes.")
    except Exception as e:
        print(f"Error: {e}")

# Main logic: check arguments and run the conversion
if len(sys.argv) != 3:
    print("Usage: python3 generate_c_array.py <input_file> <array_name>")
else:
    convert_to_c_array(sys.argv[1], sys.argv[2])
