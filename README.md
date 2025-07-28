# C-Polygon
<BR>

 <ins> **Brief**  </ins>

C Polygon is a polygon file (.ply) parser written in C89 and x64 assembly. It is lightweight, portable (tested on MSVC and GCC), and compiles for both C and C++.

<BR>

 <ins> **Ply File Structure**  </ins>

C Polygon considers a .ply file as a file comprised of 2 main parts:

- A header, with a beginning, format, comments, obj_infos, elements, properties, and an end.
- Data, which matches the description set in the header.

Format and version information is required in the file header. The format keyword must only be defined only once and may have one of three values:
```ascii, binary_little_endian, binary_big_endian```
Directly after this is the version. Unless specified otherwise in ```PlyLoadInfo``` by ```.allowAnyVersion = true```, the only valid version is ```1.0```.

C Polygon expects elements to have a unique name followed it's instance count.

```element <name> <count>``` 

Directly below the element's declaration are properties which define the type of layout of the data in the instances of that element.

```property <scalar_type> <name>```

```property <list> <list_count_type> <scalar_type> <name>```

Comments begin with the comment keyword, followed by a string.

```comment <string>``` 

Obj_infos begin with the obj_info keyword, followed by a double precision float.

```obj_info <d64>```

A complete file should be structured like the one below:
```
ply
format ascii 1.0
element vertex 8
property float x
property float y
property float z
element face 6
property list uchar int vertex_indices
end_header
-1 -1 -1 
1 -1 -1 
1 1 -1 
-1 1 -1 
-1 -1 1 
1 -1 1 
1 1 1 
-1 1 1 
4 0 1 2 3 
4 5 4 7 6 
4 6 2 1 5 
4 3 7 4 0 
4 7 3 2 6 
4 5 1 0 4 
```

<ins> **Scalar Types** </ins>

**Note:** Some .ply parsers allow for the bitcount to follow the name of a scalar type e.g. `char8, short16, etc..`. C_Polygon can load files that use this naming convention, but will not respect the bitcount at the end of the name. By this, I mean that `uint8` will be read as `uint`, which is 32 bits, not 8.

|         Name  |          Type              |         Bytes |
| ------------- | -------------------------- | ------------- |
| char          | character                  | 1
| uchar         | unsigned character         | 1
| short         | short integer              | 2
| ushort        | short unsigned integer     | 2
| int           | integer                    | 4
| uint          | unsigned integer           | 4
| float         | floating-point value       | 4
| double        | double-precision float     | 8


<ins> **Limitations** </ins>

**Max File Size:** UINT64_MAX-1

**Max Line Length:** C_PLY_MAX_LINE_LENGTH (Default: (uint32_t)200000lu)

**Max Property/Element Name Length:** PLY_MAX_ELEMENT_AND_PROPERTY_NAME_LENGTH (Default: (uint16_t)127u)



<ins> **Performance** </ins>

Average time of file parsing over 10 iterations, as measured on an Alienware M18 with an Intel i9 @ 2.2 GHZ
|         Name           | File Size              | Vertex Count       | Index Count       |         Time (Sec) |
| --------------------   | ---------------------- | ------------------ | ----------------- | ------------------ |
| lucy.ply               | 508.36 MB              | 14027872           | 28055742          | 0.563              |
| xyzrgb_dragon.ply      | 130.81 MB              | 3609600            | 7219045           | 0.156              |
| bun000.ply             |    1.9 MB              | 40256              | 204800            | 0.02              |


<ins> **Naming Conventions** </ins>
- Preprocessor Macros: UPPER_SNAKE_CASE
- Function Names: CamelCase
- Variable names: pascalCase
- Enum Types: CamelCase
- Enum Values: UPPER_SNAKE_CASE
