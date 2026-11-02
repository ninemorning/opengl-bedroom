# OpenGL Snow Cabin

한양대학교 ERICA 컴퓨터그래픽스 팀ONE 프로젝트

OpenGL로 눈 내리는 밤의 크리스마스 통나무집을 렌더링하고, 1인칭 시점으로 집 안을 돌아다니며 조명을 조작할 수 있는 프로젝트입니다. 창밖에는 눈이 내리고, 실내에는 벽난로 불꽃과 크리스마스 트리가 있습니다.

## 주요 기능

- 1인칭 카메라 이동 (WASD + 마우스), 방 경계 제한
- 바닥 / 벽 / 석재 텍스처 매핑
- 다중 점광원 조명 (천장 조명 + 스탠드 조명 + 벽난로 불빛, 불빛은 시간에 따라 깜빡임)
- 벽난로 불꽃: 노이즈 기반 프래그먼트 셰이더 + 가산 혼합(additive blending)
- 창밖에 내리는 눈: 4,000개 파티클, 위치를 버텍스 셰이더에서 실시간 계산
- 반투명 창문 유리
- 침대, 소파, 러그와 테이블, 책상, 의자, 스탠드, 옷장, 문
- OBJ / MTL 모델 로더 (src/Model.h): 재질별로 메시를 나누어 렌더링
- 크리스마스 트리 (외부 OBJ 모델, 약 23만 삼각형, 15개 재질)
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

## 진행 상황

완료: 카메라, 다중 조명, 텍스처, 가구, 창문, 벽난로 불꽃, 눈 파티클, OBJ 모델 로더, 크리스마스 트리

진행 중: 충돌 감지 (가구 통과 방지)

예정: 트리 장식 조명(컬러 점광원), 선물 상자, 추가 모델(눈사람), 그림자(시간이 허락하면)

## 폴더 구조
opengl-bedroom/
├─ src/       main.cpp, Shader.h, Camera.h, Model.h, glad.c
├─ shaders/   basic.vert, basic.frag, flame.vert, flame.frag, snow.vert, snow.frag
├─ assets/
│  ├─ textures/
│  └─ models/christmas_tree/   obj.obj, obj.mtl, 그라디언트 텍스처 3장
├─ include/   GLFW, glad, KHR, glm, stb_image.h
└─ lib/       glfw3.lib

## 에셋 출처

- 바닥 / 벽 텍스처: [Poly Haven](https://polyhaven.com) (CC0)
- 크리스마스 트리 모델: "Christmas tree stylized low-poly free", 제작자: [ogilko], CGTrader, 라이선스: CGTrader Royalty Free License (no AI)
