# OpenGL Bedroom

한양대학교 ERICA 컴퓨터그래픽스 팀ONE 프로젝트

OpenGL로 작은 침실을 렌더링하고, 1인칭 시점으로 방 안을 돌아다니며 조명을 조작할 수 있는 프로젝트입니다.

## 주요 기능

- 1인칭 카메라 이동 (WASD + 마우스)
- 바닥 / 벽 텍스처 매핑
- Phong 조명 (천장 조명 + 스탠드 조명)
- 침대, 책상, 의자, 스탠드
- 키보드로 조명 On / Off

## 조작 방법

| 키 | 기능 |
|---|---|
| W / A / S / D | 이동 |
| 마우스 | 시점 회전 |
| K | 천장 조명 On / Off |
| L | 스탠드 조명 On / Off |
| ESC | 종료 |

> 한글 / 중국어 입력기가 켜져 있으면 키 입력이 안 될 수 있습니다. 실행 후 영문 입력 상태로 바꿔 주세요.

## 개발 환경

- Windows, Visual Studio 2022 이상 (x64)
- OpenGL 3.3 Core
- GLFW, GLAD, GLM, stb_image (모두 저장소에 포함)

## 빌드 및 실행

1. 저장소를 클론합니다.
2. `opengl-bedroom/opengl-bedroom.slnx` 를 Visual Studio로 엽니다.
3. 상단 구성을 **Debug / x64** 로 선택합니다.
4. F5로 실행합니다.

라이브러리 경로는 `$(ProjectDir)` 기준 상대 경로로 설정되어 있어 별도 설정 없이 빌드됩니다.
셰이더와 텍스처는 프로젝트 폴더 기준 상대 경로로 불러오므로 Visual Studio에서 실행해야 합니다.

## 폴더 구조
opengl-bedroom/
├─ src/ main.cpp, Shader.h, Camera.h, glad.c
├─ shaders/ basic.vert, basic.frag
├─ assets/ textures/
├─ include/ GLFW, glad, KHR, glm, stb_image.h
└─ lib/ glfw3.lib

## 에셋 출처

- 바닥 / 벽 텍스처: [Poly Haven](https://polyhaven.com) (CC0)
