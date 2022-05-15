Install OpenCV
=================

- [**링크**](https://opencv.org/releases/) 에서 OpenCV 최신 버전 (현재는 4.5.5) 다운로드

- CMake GUI를 이용하여 configure 및 generate (core 및 imgproc 만 필요)

- Visual Studio 2022로 generate된 .sln 파일 실행

- CMake 타겟 중 **"ALL BUILD"** 실행

- 그 이후, **INSTALL** 타겟 실행

- 생성된 install 파일 내의 모든 내용을 ***C:\opencv*** 폴더에 복사

- 환경변수의 Path에 ***C:\opencv\x64\vc17\bin*** 추가