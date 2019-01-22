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
3. 由远及近渲染每个透明物体, 每个物体先渲染 back face, 再渲染 front face.

## Simple Font

Not Simple...

通过实现 Direct2D 与 Direct3D 互操作性, 然后配合 DWrite 在程序写入文字. 在系统不支持Direct3D 11.1的情况下, DXGI的版本为1.1. 而DXGI1.1只能通过Direct3D 10.1进行互操作. 所以这里又使用了 Surface Sharing 机制, 在 D3D11 中使用 D2D.

Surface sharing: This way, we can use direct2d with a direct3d 10.1 device to render to a surface, then using the direct3d 11 device, render that shared surface (which direct3d 10.1 and direct2d renders to) onto a square in screen space which overlays the entire scene.

参考链接:

- https://www.braynzarsoft.net/viewtutorial/q16390-14-simple-font
- https://www.cnblogs.com/X-Jun/p/9106518.html

## Direct Input

使用 Direct Input API 处理用户输入.

Direct Input uses relative mouse coordinates. What this means is it detects how much the mouse has moved since the last time it was checked.

## Skybox

可以使用 Terragen 去制作天空盒所需的六张图片. 然后使用 DirectX Texture Tool 去制作一个 3D Texture.

## Spot Light

```
struct SpotLight
{
    float3 pos;
    float innerCutOff;
    float3 dir;
    float outerCutOff;
    float3 att;
    float range;

    float4 ambient;
    float4 diffuse;
    float4 specular;
};
```

结合了 [LearnOGL](https://learnopengl-cn.github.io/02%20Lighting/05%20Light%20casters/) 中的 innerCutOff, outerCutOff 和 [Brayzarsoft](https://www.braynzarsoft.net/viewtutorial/q16390-21-spotlights)的 range, 并且参考[OgreWiki](http://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation) 中的 attenuation 参数设置.

## D3D 与 OGL 的不同点

1. 左手系. 向量叉乘按照左手螺旋定则.
2. 纹理坐标(0, 0)在左上角.
3. 默认向量表示为行向量, 矩阵右乘向量.
4. 矩阵传入着色器需要转置. (为啥??? ) [read1](https://blog.csdn.net/weiyuxinyuan/article/details/78295969), [read2](https://docs.microsoft.com/zh-cn/windows/desktop/direct3dhlsl/dx-graphics-hlsl-per-component-math)
	
5. 默认情况下, d3d cull counter-clockwise faces. 即, 照相机看到的三角形按照顺时针顶点定义.



