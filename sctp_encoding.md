# SCTP Encoding Specification

This document defines the binary format for the Simple Compact Transaction Protocol (SCTP).

## Overview

A SCTP stream is a sequence of typed data fields. Each field is prefixed with a 1-byte header that defines its type and provides metadata. All multi-byte integer and floating-point values are encoded in **little-endian** byte order.

### Header Byte

Every data field begins with a header byte, structured as follows:

`MMMM TTTT`

-   **`TTTT` (lower 4 bits):** The **Type Identifier**. This specifies the data type of the field.
-   **`MMMM` (upper 4 bits):** **Metadata**. Its meaning depends on the `TTTT` value.

---

## Type Identifiers

The `TTTT` bits map to the following data types.

| Type ID | Name      | Description                               |
| :------ | :-------- | :---------------------------------------- |
| 0       | `INT8`    | Signed 8-bit integer.                     |
| 1       | `UINT8`   | Unsigned 8-bit integer.                   |
| 2       | `INT16`   | Signed 16-bit integer.                    |
| 3       | `UINT16`  | Unsigned 16-bit integer.                  |
| 4       | `INT32`   | Signed 32-bit integer.                    |
| 5       | `UINT32`  | Unsigned 32-bit integer.                  |
| 6       | `INT64`   | Signed 64-bit integer.                    |
| 7       | `UINT64`  | Unsigned 64-bit integer.                  |
| 8       | `ULEB128` | Unsigned LEB128-encoded integer.          |
| 9       | `SLEB128` | Signed LEB128-encoded integer.            |
| 10      | `FLOAT32` | 32-bit floating-point number.             |
| 11      | `FLOAT64` | 64-bit floating-point number.             |
| 12      | `SHORT`   | A small integer (0-15) in a single byte.  |
| 13      | `VECTOR`  | A generic byte array.                     |
| 14      |           | *(Reserved for future use)*               |
| 15      | `EOF`     | End of Stream marker.                     |

---

## Type Encoding Details

### Fixed-Size & Variable-Size Numerics

-   **Types:** `INT8` - `UINT64`, `ULEB128`, `SLEB128`, `FLOAT32`, `FLOAT64`
-   **Encoding:** The header's `MMMM` bits are unused and should be `0000`. The binary data of the specified type immediately follows the header byte.

### `SHORT`

-   **Description:** Encodes a small, unsigned integer value from 0 to 15.
-   **Encoding:** The entire field is a **single byte**.
    -   The `TTTT` bits identify the type as `SHORT`.
    -   The `MMMM` bits contain the actual integer value.

### `VECTOR`

-   **Description:** Encodes a variable-length array of bytes.
-   **Encoding:** The `MMMM` bits in the header determine how the size is encoded.

    -   **Case 1: Small Vector (0-14 bytes)**
        -   The `MMMM` bits hold the length of the vector (`0000` to `1110`).
        -   The vector's byte data immediately follows the header.

    -   **Case 2: Large Vector (>= 15 bytes)**
        -   The `MMMM` bits are set to `1111`.
        -   This signals that a **ULEB128-encoded integer** representing the vector's true length follows the header.
        -   The vector's byte data follows the ULEB128 length.

### `EOF`

-   **Description:** Marks the end of the data stream.
-   **Encoding:** A single byte where the `TTTT` bits are `1111`. The `MMMM` bits should be `0000`.

