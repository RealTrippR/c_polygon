# C-Polygon
<BR>

 <ins> **Brief**  </ins>

C Polygon is a polygon file (.ply) parser written in C89 and x86 assembly. It is lightweight, portable (tested on MSVC and GCC), and compiles for both C and C++.

<BR>

 <ins> **Ply File Structure**  </ins>

C Polygon considers a .ply file as a file with 2 main parts:

- A header, comprised of a beginning, format, elements, properties, and an end.
- Data, which should match the description set in the header.

C Polygon expects elements to have a unique name followed it's instance count.

```element <name> <count>``` 

Directly below the element's declaration are properties which define the type of layout of the data in the instances of that element.

```property <scalar_type> <name>```

```property <list> <list_count_type> <scalar_type> <name>```


<ins> **Example Program**  </ins>
```

```

<ins> **Performance** </ins>

<ins> **Contributing Guidelines** </ins>

**Naming Conventions**
- Preprocessor Macros: UPPER_SNAKE_CASE
- Function Names: Camel Case
- Variable names: pascalCase
- Enum Types: Camel Case
- Enum Values: UPPER_SNAKE_CASE
