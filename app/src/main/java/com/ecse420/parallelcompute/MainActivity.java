package com.ecse420.parallelcompute;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.provider.MediaStore;
import android.Manifest;
import android.widget.Switch;
import android.view.View;

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

import android.widget.Toast;
import android.widget.Spinner;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;

import com.ecse420.parallelcompute.databinding.ActivityMainBinding;

import java.io.ByteArrayOutputStream;

public class MainActivity extends AppCompatActivity {

    private ActivityMainBinding bo;
    private Switch aSwitch;
    private static final int CAMERA_REQUEST_CODE = 100;
    private static final int CAMERA_PERMISSION_CODE = 101;

    private Spinner transformationSpinner;
    private String selectedTransformation = "Grayscale";

    private String[] transformations = {"Grayscale", "Blur", "Rotate 90°", "Scale Down"};
    
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
        setupTransformationSpinner();
        onSwitchPressed();
        onCameraClick();
    }

    private void setupTransformationSpinner() {
        transformationSpinner = bo.transformationSpinner;

        ArrayAdapter<String> adapter = new ArrayAdapter<>(
            this, android.R.layout.simple_spinner_item, transformations
        );
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        transformationSpinner.setAdapter(adapter);

        transformationSpinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                selectedTransformation = parent.getItemAtPosition(position).toString();
                Toast.makeText(MainActivity.this, "Selected: " + selectedTransformation, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                selectedTransformation = "Grayscale";
            }
        });
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

            long cpuStart = android.os.Debug.threadCpuTimeNanos();

            Bitmap transformed;
            switch (selectedTransformation) {
                case "Blur":
                    transformed = applyBlur(photo);
                    break;
                case "Rotate 90°":
                    transformed = rotateImage(photo, 90);
                    break;
                case "Scale Down":
                    transformed = scaleImage(photo, 0.5f);
                    break;
                case "Grayscale":
                default:
                    transformed = toGrayscale(photo);
                    break;
            }

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

    private Bitmap applyBlur(Bitmap original) {
        int width = original.getWidth();
        int height = original.getHeight();
        Bitmap blurred = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        
        for (int y = 1; y < height - 1; y++) {
            for (int x = 1; x < width - 1; x++) {
                int avgR = 0, avgG = 0, avgB = 0;
                
                // 3x3 kernel
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int pixel = original.getPixel(x + kx, y + ky);
                        avgR += (pixel >> 16) & 0xff;
                        avgG += (pixel >> 8) & 0xff;
                        avgB += pixel & 0xff;
                    }
                }
                
                avgR /= 9;
                avgG /= 9;
                avgB /= 9;
                
                int newPixel = 0xFF000000 | (avgR << 16) | (avgG << 8) | avgB;
                blurred.setPixel(x, y, newPixel);
            }
        }
        return blurred;
    }
    
    private Bitmap rotateImage(Bitmap original, int degrees) {
        int width = original.getWidth();
        int height = original.getHeight();
        
        Bitmap rotated = Bitmap.createBitmap(height, width, Bitmap.Config.ARGB_8888);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                rotated.setPixel(y, width - 1 - x, original.getPixel(x, y));
            }
        }
        return rotated;
    }
    
    private Bitmap scaleImage(Bitmap original, float scaleFactor) {
        int newWidth = (int)(original.getWidth() * scaleFactor);
        int newHeight = (int)(original.getHeight() * scaleFactor);
        
        Bitmap scaled = Bitmap.createBitmap(newWidth, newHeight, Bitmap.Config.ARGB_8888);
        
        for (int y = 0; y < newHeight; y++) {
            for (int x = 0; x < newWidth; x++) {
                int srcX = (int)(x / scaleFactor);
                int srcY = (int)(y / scaleFactor);
                scaled.setPixel(x, y, original.getPixel(srcX, srcY));
            }
        }
        return scaled;
    }
}