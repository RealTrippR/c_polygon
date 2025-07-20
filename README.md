# C-Polygon
<BR>

C Polygon is a polygon file (.ply) parser written in C89 and x86 assembly. It is lightweight, portable (tested on MSVC and GCC), and compiles for both C and C++.

<BR>

**Example**
```

```

**Ply File Structure**

C Polygon considers a .ply file as a file with 2 main parts:

- A header, comprised of a beginning, format, elements, properties, and an end.
- Data, which should match the description set in the header.

C Polygon expects an element to have a unique name followed it's instance count. ```element <name> <count>``` 

Directly below the element's declaration are properties which define the type of layout of the data in the instances of that element. 
```property <scalar_type> <name>```
```property <list> <list_count_type> <scalar_type> <name>```


