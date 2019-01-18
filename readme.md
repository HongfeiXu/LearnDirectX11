# DirectX 11

参考链接:

- https://www.braynzarsoft.net/viewtutorial/q16390-braynzar-soft-directx-11-tutorials

## 实验环境

Visual Studio Community 2017 + [Microsoft DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812)

注1: [Where is the DirectX SDK?](https://docs.microsoft.com/zh-cn/windows/desktop/directx-sdk--august-2009-)


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

![](images/pipeline.png)

## World, View and Local Spaces

参考链接

- https://www.braynzarsoft.net/viewtutorial/q16390-8-world-view-and-local-spaces-static-camera
- https://docs.microsoft.com/en-us/windows/desktop/direct3d11/overviews-direct3d-11-resources-buffers-intro

### Constant Buffers

A constant buffer is basically a structure in an effect file which holds variables we are able to update from our game code. We can create a constant buffer using the cbuffer type. An example, and the one we will use looks like this:

```c++
cbuffer cbPerObject
{
    float4x4 WVP;
};
```

Constant buffers should be seperated by how often they are updated. This way we can call them as least as possible since it takes processing time to do it. Examples of different frequencies we call a constant buffer are:

1. per scene (call the buffer only once per scene, such as lighting that does not change throughout the scene)
2. per frame (such as lighting that does change position or something every frame, like the sun moving across the sky)
3. per object (Like what we will do. We update the WVP per object, since each object has a different World space, or position, rotation, or scaling)

## Blending

在第12节展示了渲染多个透明物体的一种常规方法, 

1. 渲染不透明物体
2. 对透明物体按照距离排序
3. 由远及近渲染每个透明物体, 每个物体先渲染 back face, 在渲染 front face.

## D3D 与 OGL 的不同点


1. 左手系. 向量叉乘按照左手螺旋定则.
2. 纹理坐标(0, 0)在左上角.
3. 默认向量表示为行向量, 矩阵右乘向量.
4. 矩阵传入着色器需要转置. (为啥??? ) [read1](https://blog.csdn.net/weiyuxinyuan/article/details/78295969), [read2](https://docs.microsoft.com/zh-cn/windows/desktop/direct3dhlsl/dx-graphics-hlsl-per-component-math)
	
5. 默认情况下, d3d cull counter-clockwise faces. 即, 照相机看到的三角形按照顺时针顶点定义.



