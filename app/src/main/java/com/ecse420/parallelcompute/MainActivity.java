package com.ecse420.parallelcompute;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.provider.MediaStore;
import android.Manifest;
import android.widget.Switch;


import androidx.activity.EdgeToEdge;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.databinding.DataBindingUtil;

import com.ecse420.parallelcompute.databinding.ActivityMainBinding;

import java.io.ByteArrayOutputStream;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding bo;
    private Switch aSwitch;
    private static final int CAMERA_REQUEST_CODE = 100;
    private static final int CAMERA_PERMISSION_CODE = 101;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
        bo = DataBindingUtil.setContentView(this, R.layout.activity_main);
        onSwitchPressed();
        onCameraClick();
    }

    private void onSwitchPressed() {
        aSwitch = bo.switchBtn;
        aSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            if (isChecked) {
                aSwitch.setText(R.string.GPU);
            } else {
                aSwitch.setText(R.string.CPU);
            }
        });
    }

    private void onCameraClick() {
        bo.button.setOnClickListener(v -> {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.CAMERA},
                        CAMERA_PERMISSION_CODE);
            } else {
                openCamera();
            }
        });
    }

    private void openCamera() {
        Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        if (intent.resolveActivity(getPackageManager()) != null) {
            startActivityForResult(intent, CAMERA_REQUEST_CODE);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == CAMERA_PERMISSION_CODE) {
            if (grantResults.length > 0 &&
                    grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                openCamera();
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode,
                                    int resultCode,
                                    @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == CAMERA_REQUEST_CODE &&
                resultCode == RESULT_OK && data != null) {

            Bitmap photo = (Bitmap) data.getExtras().get("data");
            assert photo != null;

            // CPU start time
            long cpuStart = android.os.Debug.threadCpuTimeNanos();

            // Grayscale transformation
            Bitmap transformed = toGrayscale(photo);

            // CPU end time
            long cpuEnd = android.os.Debug.threadCpuTimeNanos();

            long cpuTime = (cpuEnd - cpuStart) / 1_000_000; // convert ns to ms

            ByteArrayOutputStream originalStream = new ByteArrayOutputStream();
            photo.compress(Bitmap.CompressFormat.PNG, 100, originalStream);
            byte[] originalBytes = originalStream.toByteArray();

            ByteArrayOutputStream transformedStream = new ByteArrayOutputStream();
            transformed.compress(Bitmap.CompressFormat.PNG, 100, transformedStream);
            byte[] transformedBytes = transformedStream.toByteArray();

            // Intent to ImageCaptured
            Intent intent = new Intent(this, ImageCaptured.class);
            intent.putExtra("original_image", originalBytes);
            intent.putExtra("transformed_image", transformedBytes);
            intent.putExtra("cpu_time", cpuTime);
            startActivity(intent);
        }
    }



    private Bitmap toGrayscale(Bitmap original) {
        int width = original.getWidth();
        int height = original.getHeight();
        Bitmap gray = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel = original.getPixel(x, y);
                int r = (pixel >> 16) & 0xff;
                int g = (pixel >> 8) & 0xff;
                int b = pixel & 0xff;
                int grayVal = (r + g + b) / 3;
                int newPixel = 0xFF000000 | (grayVal << 16) | (grayVal << 8) | grayVal;
                gray.setPixel(x, y, newPixel);
            }
        }
        return gray;
    }

}