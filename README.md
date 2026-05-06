# 🚀 Orbbec Windows Recorder (Easy Setup)

This tool is designed to record video from your Orbbec camera directly to your Windows laptop. It is specially optimized to run smoothly on older hardware (like your i3 laptop) and take advantage of your 1TB storage.

---

## 🛠 How It Works
The recorder follows a simple, automatic loop:

1.  **Connect**: It looks for your Orbbec camera.
2.  **Record**: It starts recording everything (Color + Depth).
3.  **1-Minute Batches**: Every 60 seconds, it saves the current recording as a `.bag` file and **immediately** starts a new one.
4.  **Disk Watcher**: It constantly checks your 1TB drive. if you have less than 10GB left, it will warn you in red text.
5.  **Repeat**: This continues forever until you press 'Q' or close the window.

---

## 📋 What you need (The "Ingredients")
Before you start, make sure these are on your Windows laptop:
1.  **Visual Studio (2019 or 2022)**: The free "Community" version works great.
2.  **OpenCV**: This handles the video processing.
3.  **Orbbec SDK**: The official Windows driver/library from Orbbec.
4.  **CMake**: The tool that "prepares" the code for Visual Studio.

---

## 🚀 Setup Steps (One-Time Only)

### 1. Prepare the Folders
Open a Command Prompt (cmd) in this folder and type:
```cmd
mkdir build
cd build
```

### 2. Configure (The "Mapping" step)
Tell the computer where your tools are. Replace the paths below with where you actually installed them:
```cmd
cmake .. -DOrbbecSDK_DIR="C:/Path/To/OrbbecSDK/lib/cmake/OrbbecSDK" -DOpenCV_DIR="C:/Path/To/OpenCV/build"
```

### 3. Build (The "Creating" step)
Turn the code into a real Windows App:
```cmd
cmake --build . --config Release
```

---

## ⚡️ How to Run (The Best Part)

### Standard Mode (With Video Preview)
Just double-click **`run_recorder.bat`**. You will see a window showing the camera feed.

### 🐢 i3 Laptop Mode (Maximum Performance) - RECOMMENDED
Because your i3-5005U is an older CPU, showing the video on screen can be slow. To record perfectly without slowing down your laptop:
1. Right-click `run_recorder.bat` -> **Edit**.
2. Change the last line to:
   `"%RECORDER_EXE%" --duration %DURATION% --output "%OUTPUT_DIR%" --no-gui`
3. Save and run. 
*This will record everything in the background without opening a video window, saving huge amounts of CPU power.*

---

## 📂 Where are my files?
All your recordings will appear in the `recordings/` folder, named like this:
`rec_20260426_180000_B1.bag` (Date_Time_BatchNumber)
