
#include "GLShaderManager.h"
#include "GLTools.h"
#include "GLFrustum.h"// 矩阵工具类 设置正投影矩阵/透视投影矩阵
#include "GLFrame.h"// 矩阵工具类 位置
#include "GLMatrixStack.h"// 加载单元矩阵  矩阵/矩阵相乘  压栈/出栈  缩放/平移/旋转
#include "GLGeometryTransform.h"// 交换管道，用来传输矩阵(视图矩阵/投影矩阵/视图投影变换矩阵)

#include "StopWatch.h"// 定时器
#include <math.h>
#include <stdio.h>

#include <glut/glut.h>

///
/*
 整体绘制思路：
 1、绘制地板
 2、绘制大球
 3、绘制随机的50个小球
 4、绘制围绕大球旋转的小球
 5、添加键位控制移动 -- 压栈观察者矩阵
 */

GLShaderManager     shaderManager;
// 两个矩阵
GLMatrixStack       modelViewMatix;// 模型视图矩阵
GLMatrixStack       projectionMatrix;// 投影矩阵
GLFrustum           viewFrustum;// 透视投影 - GLFrustum类
GLGeometryTransform transformPipeline;// 几何图形变换管道

GLBatch             floorTriangleBatch;// 地板
GLTriangleBatch     bigSphereBatch;// 大球体
GLTriangleBatch     sphereBatch;// 小球体

// 设置角色帧，作为相机
GLFrame  cameraFrame;// 观察者位置


// 小球门
//GLFrame sphere;
#define SPHERE_NUM  50
GLFrame spheres[SPHERE_NUM];// 50个小球



// 初始化 设置
void SetupRC() {
    
    // 初始化
    glClearColor(0, 0.3, 0.5, 1);
    shaderManager.InitializeStockShaders();
    // 开启深度测试 -- 球体转动
    glEnable(GL_DEPTH_TEST);
    
    // 地板的顶点数据
    floorTriangleBatch.Begin(GL_LINES, 324);// line 方式绘制连接，324个顶点
    for (GLfloat x = -20.f; x <= 20.f; x += 0.5f) {
        floorTriangleBatch.Vertex3f(x, -0.5f, 20.f);
        floorTriangleBatch.Vertex3f(x, -0.5f, -20.f);
        floorTriangleBatch.Vertex3f(20.f, -0.5f, x);
        floorTriangleBatch.Vertex3f(-20.f, -0.5f, x);
    }
    floorTriangleBatch.End();
    
    // 设置大球模型
    gltMakeSphere(bigSphereBatch, 0.4f, 40, 80);// 半径，切40
    
    // 设置小球模型
    gltMakeSphere(sphereBatch, 0.1f, 13, 26);
    // 小球们的位置
//    sphere.SetOrigin(-1.f,0.0f,-1.f);
    for (int i=0; i<SPHERE_NUM; i++) {
        // y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.1f);
        // 设置球的位置
        spheres[i].SetOrigin(x,0.0f,z);
    }
    
}



// 渲染
void RenderScene(void) {
    
    // 清除窗口 颜色、深度缓冲区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 颜色们
    static GLfloat vFloorColor[] = {0.0, 0.5, 0.5, 1};// 地板颜色
    static GLfloat vBigSphereColor[] = {0.3, 0.5, 0.5, 1};// 大球颜色
    static GLfloat vSphereColor[] = {0.5, 0.5, 0.7, 1};// 小球颜色

    // 定时器时间 动画 --> 大球自传
    static CStopWatch rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    
    modelViewMatix.PushMatrix();// 压栈 --> copy 一份栈顶矩阵 --> 单元矩阵
    
    
    // 观察者矩阵压栈
    /*
     观察者的移动要影响到所有绘制物体，所以要 先压栈
     */
    M3DMatrix44f cameraM;
    cameraFrame.GetCameraMatrix(cameraM);
    modelViewMatix.PushMatrix(cameraFrame);
    
    
    // 地板
    shaderManager.UseStockShader(GLT_SHADER_FLAT,transformPipeline.GetModelViewProjectionMatrix(),vFloorColor);
    floorTriangleBatch.Draw();
    
    
    // 大球
    M3DVector4f vLightPos = {0.0f,10.0f,5.0f,1.0f};// 点光源位置
    modelViewMatix.Translate(0, 0, -3.0f);// 向屏幕里面(-z)移动 3 --> 只移动一次
    
    modelViewMatix.PushMatrix();// 压栈 -- 模型视图矩阵(经过了一次平移的结果矩阵)
    modelViewMatix.Rotate(yRot, 0, 1, 0);// yRot：旋转角度 沿Y轴转动
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vBigSphereColor);// GLT_SHADER_POINT_LIGHT_DIFF：电光源着色器；vLightPos：光源位置；vBigSphereColor：绘制颜色
    bigSphereBatch.Draw();
    // 绘制大球后 pop出大球的绘制矩阵
    modelViewMatix.PopMatrix();
    
    
    // 随机摆放的小球们
//    modelViewMatix.PushMatrix();
//    modelViewMatix.MultMatrix(sphere);
//    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
//    sphereBatch.Draw();
//    modelViewMatix.PopMatrix();
    for (int i=0; i<SPHERE_NUM; i++) {
        modelViewMatix.PushMatrix();
        modelViewMatix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
        sphereBatch.Draw();
        
        modelViewMatix.PopMatrix();
    }
    
    // 围绕大球旋转的小球
    /*
     --> 为什么没有压栈呢？？？ --> 此球是绘制的最后一步了，它的绘制不会影响其他(后面没有其他了)，所以没有必要压栈
     压栈的目的是绘制 不同物体时 不同的矩阵变换 不要彼此影响
     */
    modelViewMatix.Rotate(yRot * -2, 0, 1, 0);// 旋转弧度 yRot * -2 --> +2、-2 旋转方向
    modelViewMatix.Translate(0.8f, 0, 0);// 小球X轴上平移一下，0.8f-->越大距离大球越远
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vSphereColor);
    sphereBatch.Draw();

    
    // 观察者矩阵出栈
    modelViewMatix.PopMatrix();

    // pop 出绘制初始的压栈
    modelViewMatix.PopMatrix();
    
    // 交换缓冲区
    glutSwapBuffers();
    
    // 重新绘制
    glutPostRedisplay();
}

// 视口  窗口大小改变时接受新的宽度和高度，其中0,0代表窗口中视口的左下角坐标，w，h代表像素
void ChangeSize(int w,int h) {
    // 防止h变为0
    if(h == 0)
        h = 1;
    
    // 设置视口窗口尺寸
    glViewport(0, 0, w, h);
    
    // setPerspective 函数的参数是一个从顶点方向看去的视场角度（用角度值表示）
    // 设置透视模式，初始化其透视矩阵
    viewFrustum.SetPerspective(35.0f, float(w)/float(h), 1.0f, 100.0f);
    
    //4.把 透视矩阵 加载到 透视矩阵对阵中
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    //5.初始化渲染管线
    transformPipeline.SetMatrixStacks(modelViewMatix, projectionMatrix);
}



// 移动
void SpecialKeys(int key, int x, int y) {
    
    float linear = 0.1f;// 步长
    float angle = float(m3dDegToRad(5.0f));// 旋转度数
    if (key == GLUT_KEY_UP) {// 平移
        cameraFrame.MoveForward(linear);
    }
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    if (key == GLUT_KEY_LEFT) {// 旋转
        cameraFrame.RotateWorld(angle, 0, 1.f, 0);
    }
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angle, 0, 1.f, 0);
    }
    
    
//  这里为何不需要 glutPostRedisplay(); 重新渲染 --> rendersence里的定时器会定时进行渲染，所以这里就没有必要了

}



int main(int argc,char* argv[]) {
    
    //设置当前工作目录，针对MAC OS X
    
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库
    
    glutInit(&argc, argv);
    /* 初始化双缓冲窗口，其中标志GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指
        双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区
     */
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    
    //GLUT窗口大小，标题窗口
    glutInitWindowSize(800,600);
    glutCreateWindow("Triangle");
    
    //注册回调函数
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    // 特殊键位控制移动
    glutSpecialFunc(SpecialKeys);
    
    
    //驱动程序的初始化中没有出现任何问题。
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }
    
    //调用SetupRC
    SetupRC();
    
    glutMainLoop();
    
    return 0;
}

