
# Obj-Detection-and-Tracking
Four tracking algorithms organized with cascade classifier or YOLOv3 for object detection owned by RuneScaping.
- **Sample Videos**
  - http://v.youku.com/v_show/id_XMzYxODcyNjEzMg==.html
  - http://v.youku.com/v_show/id_XMzYxODcxODY2OA==.html
  - https://youtu.be/zZZoJIJmQd8
  - https://youtu.be/rImqJhreMy4

**Requirements**: OpenCV > 3.1.0

## Functionalities
The repository is organized in different modules. Each module has a specific functionality as per below-

## [cascade_staple]
- Detector：Cascade Classifier
- Tracker：staple

## [cascade_TM](cascade_TM)
- Detector：Cascade Classifier
- Tracker：Template tracker

## [yolo_TM](yolo_TM)
- Requires: Manual compilation[darknet](https://github.com/AlexeyAB/darknet)，please place the generated darknet.so to /usr/lib/ directory.
- Detector：YOLOv3
- Tracker：Template tracker

## [yolo_KCF](yolo_KCF)
- Requires: Compilation with OpenCV_contrib
- Requires: Manual compilation[darknet](https://github.com/AlexeyAB/darknet)，please place the generated darknet.so to /usr/lib/ directory.
- Detector：YOLOv3
- Tracker：KCF

## [track_TM](track_TM)
- Tracker：Template tracker

## [track_staple](track_staple)
- Tracker：staple

## [track_opencv](track_opencv)
- Requires: Compilation with OpenCV_contrib
- Tracker：`BOOSTING`, `MIL`, `KCF`, `TLD`,`MEDIANFLOW`, `GOTURN`, `CSRT`

## [color_detect](color_detect)
- Color Detector

## [serial_linux](serial_linux)
- Linux Serial Communication Wrapper

## [serial_windows](serial_windows)
- Windows Serial Communication Wrapper