# DirectX 11

参考链接:

- https://www.braynzarsoft.net/viewtutorial/q16390-braynzar-soft-directx-11-tutorials

## Pragrammable Graphics Rendering Pipeline

参考链接

- https://docs.microsoft.com/en-us/windows/desktop/direct3d11/overviews-direct3d-11-graphics-pipeline
- https://www.braynzarsoft.net/viewtutorial/q16390-4-begin-drawing

The rendering pipeline is the set of steps direct3d uses to create a 2d image based on what the virtual camera sees. It consists of the 7 Stages used in Direct3D 10, along with 3 new stages accompanying Direct3D 11, which are as follows:

1. Input Assembler (IA) Stage
2. Vertex Shader (VS) Stage
3. Hull Shader (HS) Stage
4. Tesselator Shader (TS) Stage
5. Domain Shader (DS) Stage
6. Geometry Shader (GS) Stage
7. Stream Output (SO) Stage
8. Rasterizer (RS) Stage
9. Pixel Shader (PS) Stage
10. Output Merger (OM) Stage

![](images\pipeline.png)



