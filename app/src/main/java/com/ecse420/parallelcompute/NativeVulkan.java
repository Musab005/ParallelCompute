package com.ecse420.parallelcompute;

public class NativeVulkan {
    static {
        System.loadLibrary("nativevulkan"); // must match your library name from CMake
    }

    public static native void initVulkan(String shaderPath);
    public static native void runComputeShader(
            java.nio.ByteBuffer inBuffer,
            int width,
            int height,
            java.nio.ByteBuffer outBuffer
    );
    public static native void cleanupVulkan();
}
