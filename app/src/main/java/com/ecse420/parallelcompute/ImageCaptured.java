package com.ecse420.parallelcompute;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;


import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class ImageCaptured extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_image_captured);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
        display();
        onDoneClicked();
    }


    private void display() {
        ImageView originalImage = findViewById(R.id.capturedImage);
        ImageView transformedImage = findViewById(R.id.processedImage);
        TextView performanceSummary = findViewById(R.id.PerformanceText);

        byte[] originalBytes = getIntent().getByteArrayExtra("original_image");
        byte[] transformedBytes = getIntent().getByteArrayExtra("transformed_image");

        // Retrieve the time & mode
        long processingTime = getIntent().getLongExtra("processing_time", 0);
        String processingMode = getIntent().getStringExtra("processing_mode");
        if (processingMode == null) {
            processingMode = "CPU"; // default if none passed
        }

        if (originalBytes != null) {
            Bitmap bitmap = BitmapFactory.decodeByteArray(originalBytes, 0, originalBytes.length);
            originalImage.setImageBitmap(bitmap);
        }

        if (transformedBytes != null) {
            Bitmap bitmap2 = BitmapFactory.decodeByteArray(transformedBytes, 0, transformedBytes.length);
            transformedImage.setImageBitmap(bitmap2);
        }

        // Display the correct label
        // e.g. "GPU time: 15 ms" or "CPU time: 30 ms"
        String summary = processingMode + " time: " + processingTime + " ms\n";

        performanceSummary.setText(summary);
    }


//    private void display() {
//        ImageView originalImage = findViewById(R.id.capturedImage);
//        ImageView transformedImage = findViewById(R.id.processedImage);
//        TextView performanceSummary = findViewById(R.id.PerformanceText);
//
//        byte[] originalBytes = getIntent().getByteArrayExtra("original_image");
//        byte[] transformedBytes = getIntent().getByteArrayExtra("transformed_image");
//
//        long cpuTime = getIntent().getLongExtra("cpu_time", 0);
//
//        if (originalBytes != null) {
//            Bitmap bitmap = BitmapFactory.decodeByteArray(originalBytes, 0, originalBytes.length);
//            originalImage.setImageBitmap(bitmap);
//        }
//
//        if (transformedBytes != null) {
//            Bitmap bitmap2 = BitmapFactory.decodeByteArray(transformedBytes, 0, transformedBytes.length);
//            transformedImage.setImageBitmap(bitmap2);
//        }
//
//        String summary = "CPU time: " + cpuTime + " ms\n";
//
//
//        performanceSummary.setText(summary);
//    }

    private void onDoneClicked() {
        Button doneBtn = findViewById(R.id.DoneBtn);
        doneBtn.setOnClickListener(v -> {
            finish();
        });
    }
}