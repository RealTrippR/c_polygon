# C-Polygon
<BR>

 <ins> **Brief**  </ins>

C Polygon is a polygon file (.ply) parser written in C89 and x86 assembly. It is lightweight, portable (tested on MSVC and GCC), and compiles for both C and C++.

<BR>

 <ins> **Ply File Structure**  </ins>

C Polygon considers a .ply file as a file with 2 main parts:

- A header, comprised of a beginning, format, comments, obj_infos, elements, properties, and an end.
- Data, which should match the description set in the header.

Format and version information is required in the file header. The format keyword must only be defined once and have 1 of the three values:
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

A complete file should have a structure like the one below:
```

ply
format ascii 1.0
comment created by platoply
element vertex 8
property float32 x
property float32 y
property float32 z
element face 6
property list uint8 int32 vertex_indices
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


<ins> **Example Program**  </ins>
```

```

<ins> **Limitations** </ins>

<ins> **Performance** </ins>

<ins> **Contributing Guidelines** </ins>

**Naming Conventions**
- Preprocessor Macros: UPPER_SNAKE_CASE
- Function Names: Camel Case
- Variable names: pascalCase
- Enum Types: Camel Case
- Enum Values: UPPER_SNAKE_CASE
