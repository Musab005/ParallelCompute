package com.ecse420.parallelcompute;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLES30;
import android.opengl.GLUtils;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class OpenGLHelper {
    private static final String TAG = "OpenGLHelper";
    
    // Shader program handles
    private int blurProgram = 0;
    
    // Framebuffer for offscreen rendering
    private int[] framebuffers = new int[1];
    private int[] textures = new int[2];  // input and output textures
    
    // Vertex data
    private static final float[] VERTICES = {
            -1.0f, -1.0f,  // bottom left
            -1.0f,  1.0f,  // top left
             1.0f, -1.0f,  // bottom right
             1.0f,  1.0f   // top right
    };
    private FloatBuffer vertexBuffer;
    
    // Shader source code
    private static final String VERTEX_SHADER =
            "#version 300 es\n" +
            "layout(location = 0) in vec2 position;\n" +
            "out vec2 texCoord;\n" +
            "void main() {\n" +
            "    gl_Position = vec4(position, 0.0, 1.0);\n" +
            "    texCoord = position * 0.5 + 0.5;\n" + // map from [-1,1] to [0,1]
            "}";
    
    private static final String BLUR_FRAGMENT_SHADER =
            "#version 300 es\n" +
            "precision mediump float;\n" +
            "in vec2 texCoord;\n" +
            "uniform sampler2D inputTexture;\n" +
            "uniform vec2 texelSize;\n" + // 1.0/width, 1.0/height
            "out vec4 fragColor;\n" +
            "void main() {\n" +
            "    vec4 sum = vec4(0.0);\n" +
            "    // 3x3 kernel\n" +
            "    for (int y = -1; y <= 1; y++) {\n" +
            "        for (int x = -1; x <= 1; x++) {\n" +
            "            vec2 offset = vec2(float(x), float(y)) * texelSize;\n" +
            "            sum += texture(inputTexture, texCoord + offset);\n" +
            "        }\n" +
            "    }\n" +
            "    // Average\n" +
            "    fragColor = sum / 9.0;\n" +
            "}";
    
    public OpenGLHelper() {
        // Initialize vertex buffer
        ByteBuffer bb = ByteBuffer.allocateDirect(VERTICES.length * 4); // 4 bytes per float
        bb.order(ByteOrder.nativeOrder());
        vertexBuffer = bb.asFloatBuffer();
        vertexBuffer.put(VERTICES);
        vertexBuffer.position(0);
    }
    
    public void init() {
        // Create shader programs
        blurProgram = createProgram(VERTEX_SHADER, BLUR_FRAGMENT_SHADER);
        
        // Generate framebuffers and textures
        GLES30.glGenFramebuffers(1, framebuffers, 0);
        GLES30.glGenTextures(2, textures, 0);
    }
    
    public Bitmap applyBlur(Bitmap input) {
        int width = input.getWidth();
        int height = input.getHeight();
        
        // Configure input texture
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, textures[0]);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);
        GLUtils.texImage2D(GLES30.GL_TEXTURE_2D, 0, input, 0);
        
        // Configure output texture
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, textures[1]);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_S, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_WRAP_T, GLES30.GL_CLAMP_TO_EDGE);
        GLES30.glTexImage2D(GLES30.GL_TEXTURE_2D, 0, GLES30.GL_RGBA, width, height, 0, 
                           GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, null);
        
        // Set up framebuffer with output texture
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, framebuffers[0]);
        GLES30.glFramebufferTexture2D(GLES30.GL_FRAMEBUFFER, GLES30.GL_COLOR_ATTACHMENT0,
                                     GLES30.GL_TEXTURE_2D, textures[1], 0);
        
        if (GLES30.glCheckFramebufferStatus(GLES30.GL_FRAMEBUFFER) != GLES30.GL_FRAMEBUFFER_COMPLETE) {
            Log.e(TAG, "Framebuffer not complete");
            return null;
        }
        
        // Set viewport to match texture dimensions
        GLES30.glViewport(0, 0, width, height);
        
        // Use blur shader
        GLES30.glUseProgram(blurProgram);
        
        // Bind input texture
        GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, textures[0]);
        GLES30.glUniform1i(GLES30.glGetUniformLocation(blurProgram, "inputTexture"), 0);
        
        // Set texel size uniform
        GLES30.glUniform2f(GLES30.glGetUniformLocation(blurProgram, "texelSize"), 
                         1.0f / width, 1.0f / height);
        
        // Draw fullscreen quad
        GLES30.glVertexAttribPointer(0, 2, GLES30.GL_FLOAT, false, 0, vertexBuffer);
        GLES30.glEnableVertexAttribArray(0);
        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
        GLES30.glDisableVertexAttribArray(0);
        
        // Read back the result
        Bitmap resultBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        ByteBuffer buffer = ByteBuffer.allocateDirect(width * height * 4);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        GLES30.glReadPixels(0, 0, width, height, GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, buffer);
        buffer.rewind();
        resultBitmap.copyPixelsFromBuffer(buffer);
        
        // Unbind framebuffer
        GLES30.glBindFramebuffer(GLES30.GL_FRAMEBUFFER, 0);
        
        return resultBitmap;
    }
    
    private int loadShader(int type, String shaderCode) {
        int shader = GLES30.glCreateShader(type);
        GLES30.glShaderSource(shader, shaderCode);
        GLES30.glCompileShader(shader);
        
        final int[] compileStatus = new int[1];
        GLES30.glGetShaderiv(shader, GLES30.GL_COMPILE_STATUS, compileStatus, 0);
        
        if (compileStatus[0] == 0) {
            Log.e(TAG, "Shader compilation error: " + GLES30.glGetShaderInfoLog(shader));
            GLES30.glDeleteShader(shader);
            return 0;
        }
        
        return shader;
    }
    
    private int createProgram(String vertexShaderCode, String fragmentShaderCode) {
        int vertexShader = loadShader(GLES30.GL_VERTEX_SHADER, vertexShaderCode);
        int fragmentShader = loadShader(GLES30.GL_FRAGMENT_SHADER, fragmentShaderCode);
        
        if (vertexShader == 0 || fragmentShader == 0) {
            return 0;
        }
        
        int program = GLES30.glCreateProgram();
        GLES30.glAttachShader(program, vertexShader);
        GLES30.glAttachShader(program, fragmentShader);
        GLES30.glLinkProgram(program);
        
        final int[] linkStatus = new int[1];
        GLES30.glGetProgramiv(program, GLES30.GL_LINK_STATUS, linkStatus, 0);
        
        if (linkStatus[0] == 0) {
            Log.e(TAG, "Program linking error: " + GLES30.glGetProgramInfoLog(program));
            GLES30.glDeleteProgram(program);
            return 0;
        }
        
        GLES30.glDeleteShader(vertexShader);
        GLES30.glDeleteShader(fragmentShader);
        
        return program;
    }
    
    public void cleanup() {
        GLES30.glDeleteProgram(blurProgram);
        GLES30.glDeleteFramebuffers(1, framebuffers, 0);
        GLES30.glDeleteTextures(2, textures, 0);
    }
}