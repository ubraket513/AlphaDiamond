# Engine/Training–Desktop Repository Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 공통 엔진 구현을 유지하면서 학습 실험과 Desktop 앱을 서로 다른 저장소에서 독립적으로 개발·검증·릴리스한다.

**Architecture:** 기존 저장소는 Engine/Training과 Runtime SDK의 원본을 소유한다. Desktop은 별도 저장소에서 정확한 버전의 설치 SDK를 소비한다. 먼저 기존 저장소 안에서 경계를 만들고 외부 소비자를 검증한 뒤 앱을 추출한다.

**Tech Stack:** 현재 C++26 dialect, CMake, CTest, LibTorch, Qt 6.6+ / QML; 기존 GitHub/Hugging Face 모델 배포 계약.

**Spec:** [의존성 분석과 분리 설계](../../architecture/2026-10-29-repository-split-analysis.md)

## Global Constraints

- 저장소는 두 개이며 공통 엔진 구현은 Engine/Training에만 둔다. 원격 이름은 생성 시 확정한다.
- Desktop은 설치 SDK의 정확한 버전과 패키지 SHA-256을 고정한다. 기본 빌드에서 Engine checkout을 요구하지 않는다.
- C++26 dialect, Qt 하한 6.6, QML preview 하한 6.12를 보존한다.
- format-v3, 기존 encoder/규칙/텐서/digest/모델 identity 의미를 유지한다. SDK 분리로 metadata.json 필드를 추가하지 않는다.
- Python을 앱/학습의 지원 빌드·테스트·런타임 의존성으로 재도입하지 않는다.
- 실제 이동/추출 전 미커밋·미추적 변경을 보존하고 확정된 커밋을 기준으로 작업한다. 기존 로컬 변경을 reset/clean하지 않는다.
- 개발 빌드는 GCC와 mold를 사용하고 Ninja 및 기존 sccache 캐시를 활용한다. Windows는 플랫폼 기본 linker를 유지한다.
- 빌드와 테스트 모두 CPU·메모리에 맞게 명시적으로 병렬 실행한다. GPU/공유 자원 테스트는 자원 잠금 또는 worker 제한을 둔다.
- 지원 코드 변경 후 `graphify update .`를 실행한다. QML/문서 의미 관계는 별도 재추출 절차로 갱신한다.
- 이 문서는 계획이며 아래 체크박스는 아직 실행하지 않았다. 문서 작성만을 이유로 새 원격 생성·push·이력 재작성·소스 삭제를 수행하지 않는다.

## Review Focus

- SDK를 이동한 뒤 원본 checkout/Torch 경로가 없어도 소비자가 빌드되는가: Task 3의 relocation test.
- 모델이 없는 설치에서 앱 시작과 보드가 동작하는가: Task 4의 topology/no-model test.
- 잘못된 family/encoder/digest 모델이 기존 설치를 훼손하지 않는가: Task 5의 모델 거절 테스트.
- shell/hf/네트워크가 없어도 rating outbox가 남고 재시도가 가능한가: Task 2/5의 rating 테스트.
- 새 저장소가 기존 사용자 캐시·저장 대국·installation ID를 이어 쓰는가: Task 6의 사용자 데이터 회귀 테스트.

## 실행 단위와 순서

Task 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7. 각각 별도 검토 가능한 변경으로 만든다.
SDK를 실제로 소비하는 Task 4가 통과하기 전에는 기존 저장소에서 Qt 소스를 삭제하지 않는다.
SDK API를 불필요하게 재설계하거나 전 소스 디렉터리를 함께 이동하지 않는다.

### Task 0: 추출 기준점과 기존 동작 확정

**Files:** 현재 Git 상태, `CMakePresets.json`, `.github/workflows/ci.yml`, `docs/architecture/2026-10-29-repository-split-analysis.md`.

**Interfaces:** consumes 현재 checkout; produces 확정 `SPLIT_BASE_SHA`, 파일 소유권 목록, 기존 테스트 결과, fixture digest 목록.

- [ ] 기존 수정·삭제·미추적 파일을 분류하고 필요한 변경을 소유자와 정리한다. 현재 HEAD만 추출하면 미커밋 작업이 누락됨을 확인한다.
- [ ] CPU 수/가용 메모리와 LibTorch 컴파일 peak 메모리를 확인해 `BUILD_JOBS`, `TEST_JOBS`를 정한다. OMP/LibTorch 내부 스레드까지 고려한다.
- [ ] 현재 core, CPU training, Qt+실제 추론 preset을 configure/build/test한다. 모델/Qt kit 부재는 실패와 구별해 기록하고 해당 플랫폼 CI에서 검증하도록 배정한다.
- [ ] CPU 학습 smoke/resume/export 및 Soo/Min artifact digest/규칙 golden의 기준 결과를 기록한다. 새 장시간 학습은 실행하지 않는다.
- [ ] 확정 커밋 SHA와 기존 데이터·설정 보존 목록을 기록한다. 이후 변경은 `codex/repository-split-*` 작업 브랜치에서 진행한다.

검증 명령 형식: `cmake --build --preset <preset> --parallel "$BUILD_JOBS"` 후 `ctest --preset <preset> --parallel "$TEST_JOBS" --output-on-failure`.
Windows에서는 기존 `tools/native_training.sh` launcher를 사용한다. 환경에 맞는 LibTorch/Qt 경로를 명시한다.
완료 기준: 경계 변경 전 실제 성공/기존 실패/환경상 미실행을 구별한 기준표가 있다.

### Task 1: 추론과 학습의 CMake 경계 분리

**Files:** modify `CMakeLists.txt`, `native/CMakeLists.txt`, `native/qt/CMakeLists.txt`, `CMakePresets.json`; create `native/tests/runtime_boundary_test.cmake`.

**Interfaces:** produces `DIAMOND_BUILD_MODEL`, `DIAMOND_BUILD_TRAINING`; `DIAMOND_BUILD_LIBTORCH`는 호환 umbrella로 남기되 신규 preset에서는 OFF로 둔다.

- [ ] 실패 테스트를 만든다: MODEL ON/TRAINING OFF 구성에서 `diamond_model`과 Qt 추론 타깃은 존재하지만 `diamond_training`, `diamond_native_pipeline`, `diamond_orchestration`, train/checkpoint/release 실행 파일은 존재하지 않아야 한다.
- [ ] `native-ci`, `native-runtime` (신규), `native-training`, `native-qt` preset의 옵션을 명시한다. TRAINING ON/MODEL OFF는 configure에서 명확히 거절한다.
- [ ] 모델 정의를 학습 블록 밖으로 옮기고 학습·release·benchmark·관련 테스트를 TRAINING 블록으로 제한한다. 기존 umbrella ON은 이전처럼 모델+학습을 활성화한다.
- [ ] Qt 추론 조건을 새 MODEL 타깃으로 검사한다. 임시로 rating이 없는 구성이 생기는 것은 Task 2 완료 전 중간 단계이며 최종 릴리스하지 않는다.
- [ ] 위 경계 테스트와 기존 core/model/training 테스트를 병렬 실행하고 변경만 커밋한다.

완료 기준: Qt 추론을 configure/build하는 데 학습 타깃 정의가 필요하지 않다. target diamond_qt만 선택해서 생기는 착시가 아닌 전체 빌드로 확인한다.

### Task 2: Support·artifact·rating을 독립 타깃으로 추출

**Files:** modify `native/CMakeLists.txt`, `native/qt/CMakeLists.txt`; reuse `native/src/{json,deployment_artifact,rating,rating_store,rating_sync_main}.cpp`와 해당 공개 헤더; modify `native/qt/tests/rating_bridge_test.cpp`.

**Interfaces:** produces `alphadiamond::{diamond_support,diamond_artifact,diamond_rating}`. 기존 C++ 함수·namespace·헤더 경로를 유지한다.

- [ ] runtime-only 구성에서 artifact 검증/색인 및 rating 이벤트/registry/outbox 테스트를 실행하도록 옮긴다. 현재 학습 블록 안에 있는 `rating_registry_test`, `rating_store_transaction_test`, `rating_sync_test`, `model_index_test`의 의존성을 조사해 최소 타깃에 연결한다.
- [ ] `json.cpp`를 support에만 편입한다. `diamond_pipeline`은 support와 core를 사용하고 model은 artifact를 사용하게 한다.
- [ ] `deployment_artifact.cpp`를 artifact, `rating.cpp/rating_store.cpp`를 rating에 편입한다. orchestration에서 해당 소스를 제거하고 rating에 링크한다.
- [ ] Qt를 orchestration 대신 rating에 링크하고 존재 여부에 따른 자동 활성화 대신 명시적인 `DIAMOND_DESKTOP_ENABLE_RATINGS` 옵션(기본 ON)을 사용한다.
- [ ] `alphadiamond-rating-sync`를 training OFF에서도 빌드 가능하게 한다. 학습과 Desktop 배포에서 같은 구현을 사용한다.
- [ ] 로컬 이벤트 기록과 중복 방지, receipt 이전 실패 복구, sync 도구 누락 시 pending 이벤트 보존 테스트를 실행한다. 기존 Qt rating bridge 테스트도 runtime-only에서 실행한다.
- [ ] graphify를 갱신하고 타깃 경계와 회귀 결과를 검토한 뒤 커밋한다.

완료 기준: Qt/rating-sync/SDK에 replay·trainer·coordinator 링크가 없다. 정적 라이브러리에서 실제로 링크된 object만 작다는 이유로 orchestration 의존을 남기지 않는다.

### Task 3: 외부 소비 가능한 Runtime SDK 릴리스

**Files:** modify 루트/native CMake; create `cmake/AlphaDiamondEngineConfig.cmake.in`, `native/tests/sdk_consumer/{CMakeLists.txt,main.cpp}`, `native/tests/sdk_install_test.cmake`, `.github/workflows/engine-sdk.yml`.

**Interfaces:** produces `AlphaDiamondEngine` package `0.1.0`, `Runtime` component, 분석 문서의 여섯 imported targets; SDK tools의 rating-sync, topology/data/fixture 및 release manifest.

- [ ] 외부 소비자 테스트를 먼저 추가한다: install prefix만 받아 `find_package(AlphaDiamondEngine 0.1.0 EXACT CONFIG REQUIRED COMPONENTS Runtime)`로 여섯 타깃에 링크하고 core/rating/artifact API를 호출한다.
- [ ] 공개 헤더 allowlist, 타깃 export, config/version 파일, Threads/Torch dependency 재탐색 및 C++26 usage requirement를 구현한다. Torch include·compile options를 producer 머신 경로로 굳히지 않는다.
- [ ] SDK manifest에 소스 SHA, SDK version, platform/arch/compiler/CRT/configuration, Torch version/variant/ABI, 지원 game/encoder/artifact 계약, 파일 SHA-256을 기록한다.
- [ ] 설치 디렉터리를 새 prefix로 복사하고 source/build 원본을 접근 불가능하게 한 뒤 외부 소비자를 configure/build/test한다. 다른 SDK 버전과 ABI 불일치도 configure에서 거절됨을 확인한다.
- [ ] 테스트용 작은 Soo/Min runtime fixture와 canonical topology를 SDK data에 포함한다. 학습 optimizer/checkpoint나 전체 training-v1 corpus는 SDK에 넣지 않는다. 각 fixture의 출처와 digest를 기록한다.
- [ ] Windows CPU SDK를 우선 검증하고 기존 core 지원 플랫폼의 소비자 검증을 유지한다. 미제공 바이너리 플랫폼에는 같은 태그 소스에서 SDK를 만드는 절차를 제공한다.
- [ ] 태그/릴리스는 모든 검증 후 발행한다. 기존 모델 artifact 버전과 SDK 태그를 혼용하지 않는다.

완료 기준: clean consumer가 relocation 후에도 설치 SDK만으로 빌드된다. SDK 릴리스에는 training 실행 파일이 포함되지 않는다.

### Task 4: 기존 저장소에서 Qt를 SDK 소비자로 전환

**Files:** modify `native/qt/CMakeLists.txt`, `native/qt/native_controller.cpp`, Qt의 model catalog/runtime 테스트; create `desktop/CMakeLists.txt`, `desktop/engine-sdk.lock.json`, `desktop/cmake/VerifyEngineSdk.cmake`, `desktop/resources/model-sources.json`, `native/qt/tests/topology_resource_test.cpp`.

**Interfaces:** consumes Task 3 SDK; produces Engine source를 포함하지 않는 standalone Desktop build. `desktop/`은 추출 전 임시 entry이며 Task 6에서 새 루트로 이동한다.

- [ ] 현재 `native/qt` 소스를 참조하되 Engine의 루트 CMake를 실행하지 않는 독립 entry를 추가한다. 프로젝트 전역 옵션/컴파일러 플래그/CTest 설정을 독립 entry에 명시한다.
- [ ] lock schema 1을 정의한다: `sdk_version`, `source_commit`, `platform`, `arch`, `compiler`, `configuration`, `torch_version`, `torch_variant`, `cxx_abi`, `archive_sha256`. 다른 플랫폼은 별도 entry로 고정한다. configure에서 선택 entry와 설치 manifest가 일치해야 한다.
- [ ] 모델 URL 설정은 기존 GitHub `ubraket513/alphadiamond`, HF `ubraket513/AlphaDiamond`를 유지한다. 모델 repository/bucket identity를 Desktop의 새 Git remote에서 유추하지 않는다.
- [ ] SDK topology를 Qt 리소스로 편입해 `NativeController::loadTopology()`가 모델 없는 상태에서도 동작하게 한다. package/index 선택 경로를 존중하고 repo/CWD의 golden/artifacts 자동 탐색은 기본 경로에서 없앤다.
- [ ] 테스트를 임의의 비어 있는 작업 디렉터리에서 실행한다. Engine checkout을 숨긴 상태에서 controller/replay/model catalog/rating 및 no-model 보드 시작을 검증한다.
- [ ] 명시적 SDK prefix만으로 Desktop 전체 빌드가 통과하고 training 타깃이 등록되지 않는지 확인한 뒤 커밋한다.

완료 기준: 기존 저장소 안에서도 앱은 외부 SDK 소비자로 동작한다. 공통 코드의 두 번째 복사본을 만들지 않는다.

### Task 5: 독립 CI와 Windows 배포 검증

**Files:** modify `.github/workflows/ci.yml`, `tools/{build_native_qt,deploy_native_qt,run_native_qt,preview_native_qt,sync_ratings}.sh`; create `tools/desktop_toolchain.sh`, `.github/workflows/desktop.yml`; modify Qt 모델/rating 테스트.

**Interfaces:** consumes 고정 SDK와 모델 bundle; produces Desktop만으로 만드는 runtime package와 Engine만으로 실행되는 training CI.

- [ ] Desktop build launcher를 `native_training.sh`에서 분리한다. Windows toolchain 초기화 중 앱에 필요한 부분만 Desktop launcher로 가져오고 원본 라이브 경로를 요구하지 않는다.
- [ ] 배포 스크립트에 `--sdk-prefix`, `--models-dir` 입력을 추가한다. SDK와 맞는 Torch DLL만 번들하고 training CUDA 환경에서 임의로 DLL을 가져오지 않는다. `models` 입력이 없는 package도 지원한다.
- [ ] rating-sync 실행 파일을 SDK tools에서 번들하고 기존 sync launcher를 배포한다. 외부 shell/hf가 없을 때 pending 보존 상태를 표시한다. shell/hf는 명시적으로 제공된 환경에서만 온라인 sync 테스트에 사용한다.
- [ ] Desktop CI에 shell UI 테스트와 실제 CPU LibTorch+Soo/Min+rating 테스트를 구분해 등록한다. Min fixture 미설정으로 추론 테스트가 조용히 빠지지 않도록 테스트 개수를 검사한다.
- [ ] family 불일치, 지원하지 않는 encoder, 변조된 tensor/digest, 중단된 다운로드를 거절하고 기존 설치 모델을 보존하는지 검증한다. 네트워크 테스트는 로컬 fixture/mock을 사용한다.
- [ ] Windows 패키지에서 source tree와 개발 PATH를 제거하고 no-model 및 Soo/Min AI·replay·sound·window smoke를 실행한다. 일반 테스트는 병렬, 창/공유 오디오 검사는 자원 잠금으로 충돌을 피한다.
- [ ] Engine CI에서 Qt kit 없이 CPU 학습/resume/export, 기존 CUDA opt-in acceptance가 유지되는지 검증한다. CTest에도 명시적인 worker 수를 적용한다.

완료 기준: SDK 고정 상태에서 앱 CI/패키징은 학습 소스와 GPU 없이 동작하고 학습 CI는 Qt 없이 동작한다.

### Task 6: Desktop 저장소 추출과 이력 보존

**Files:** 새 Desktop의 `app/`, `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `tools/`, `resources/`, `engine-sdk.lock.json`, `.github/`, `README.md`, `AGENTS.md`, 라이선스/서체 고지.

**Interfaces:** consumes Task 5까지 검증된 확정 커밋; produces 두 독립 checkout과 원본→새 경로/커밋 매핑 manifest.

- [ ] 별도 disposable clone에서만 경로 필터링을 수행한다. 원본 checkout/원격의 이력을 재작성하지 않는다. `native/qt/`→`app/`, 임시 `desktop/`→루트, 앱 전용 tools/docs/CI/라이선스를 보존하는 명시적 allowlist를 만든다.
- [ ] 공통 루트 CMake/정책/README는 혼합 내용 전체를 복사하지 않고 Desktop 전용 버전으로 작성한다. 명령/파일 경로를 새 레이아웃에 맞춘다.
- [ ] topology/minimal fixture는 SDK 출처와 digest를 유지한다. Desktop hygiene는 학습 `tests/golden/MANIFEST.json`의 존재를 요구하지 않고 SDK fixture manifest를 검증한다.
- [ ] 기존 QCoreApplication 조직명/앱 이름/사용자 데이터 위치를 유지한다. 기존 save 파일, 모델 캐시, rating installation ID/outbox를 담은 임시 사용자 profile에서 읽기/쓰기 회귀를 검사한다.
- [ ] 새 checkout만으로 Task 5의 configure/build/test/package를 재실행한다. Engine 형제 디렉터리·이전 source path가 없어도 통과해야 한다.
- [ ] 두 저장소의 AGENTS/Serena project root/Graphify를 각각 설정하고 그래프를 재생성한다. 이전 전체 그래프를 복사하지 않는다.
- [ ] 검증된 새 Desktop 초기 상태와 SHA 매핑을 기록한 뒤 원격을 생성하고 push한다. 실패하면 기존 저장소의 Qt 개발/릴리스 경로를 계속 사용한다.

완료 기준: 별도 checkout의 CI가 통과한다. 이력 추출에 기존 미커밋/ignored 데이터가 유입되지 않는다.

### Task 7: 기존 저장소 정리와 독립 릴리스 전환

**Files:** Engine의 `native/qt/` 및 Qt 전용 tools/CI/presets 제거; 양쪽 README, 제품/모델 계약 문서, Makefile, `cmake/Verify*.cmake`, 분리 안내 문서 갱신.

**Interfaces:** consumes 별도 Desktop의 검증된 릴리스; produces Engine-only 기존 저장소와 SDK 업그레이드 절차.

- [ ] 새 Desktop 릴리스의 배포 smoke와 사용자 데이터 회귀가 통과한 뒤에만 기존 저장소에서 Qt 파일/전용 타깃을 제거한다.
- [ ] 모델 배포 index/GitHub/HF 주소는 Engine에 유지한다. 기존 사용자 앱의 다운로드 URL이 바뀌지 않게 확인한다.
- [ ] 원본 규칙/encoder/모델 계약 문서는 Engine에 두고 Desktop에서 링크한다. 과거 Python exporter 경로 및 더 이상 없는 Make target 설명을 정리한다.
- [ ] training-only checkout의 전체 검증과 Desktop 고정 SDK 검증을 각각 실행한다. 앱 UI 수정은 SDK 릴리스 없이 배포 가능하고 trainer 수정은 Desktop lock을 바꾸지 않음을 확인한다.
- [ ] SDK 업그레이드는 새 버전·digest·ABI lock 변경과 앱 호환 테스트를 묶은 별도 변경으로 운영한다. 새 encoder/architecture 실험은 새 SDK 지원 전까지 기존 앱에 자동 채택하지 않는다.
- [ ] rollback 기준을 기록한다: SDK 문제는 Desktop lock을 이전 버전으로 되돌리고, 모델 문제는 이전 index/default를 선택한다. 기존 분리 기준 커밋과 추출 매핑은 보존한다.

최종 검토는 분석 문서의 일곱 완료 조건으로 진행한다. 실제 소스 변경·저장소 생성은 이 계획 검토 이후 별도 실행 작업이다.
