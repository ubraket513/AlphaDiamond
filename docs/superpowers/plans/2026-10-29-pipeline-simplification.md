# Pipeline Simplification and File Commentary Implementation Plan

> **For agentic workers:** Use the already authorized local/subagent execution. The latest human instructions below override earlier test-driven or approval-gated workflow suggestions. Do not add tests or CI for this cleanup.

**Goal:** 기존 코드를 직접 읽기 쉽게 줄이고 상세한 파일별 주석을 작성한다.

**Architecture:** 기존 함수와 모듈을 직접 정리한다. 새 실행 계층, manager, wrapper, routing policy, fallback, compatibility framework를 만들지 않는다.

**Tech Stack:** 기존 C++/LibTorch, CMake/Ninja, GCC/mold/sccache. Serena/Graphify로 실제 참조 확인.

**Spec:** `docs/superpowers/specs/2026-10-29-pipeline-simplification-design.md`

## 사용자 최종 지시

- 사용자가 필요한 테스트와 GitHub CI를 나중에 별도로 처음부터 만든다. 지금은 신규/임시 회귀 테스트도, 대체 CI도 만들지 않는다.
- 기존 것을 포함한 회귀 테스트 전체와 기존 GitHub CI를 제거한다. 테스트 전용 데이터·도구·등록·실행 참조까지 정리한다.
- 검증은 남은 제품/벤치마크의 빌드와 실제 CLI·패널 실행으로 한다. 이미 수행한 검증 결과는 기록으로 보존한다.
- 상세 소스 주석은 유지한다. 책임·입출력·소유권·부작용·숨은 정책을 정확히 설명한다.
- 최신 사용자 지시로 훈련·평가를 중지하고 코드 정리를 즉시 시작한다. 미완료 평가를 재개하지 않으며 체크포인트·실험 결과·사용자 변경은 보존한다.
- 로컬 브랜치에서 코드 작업을 완료한다. 자동 예약은 삭제했으며 훈련·평가와 자동 메시지를 재개하지 않는다.

## 1. 설정 연결과 중복 정리

- [x] 기존 config/training_wiring/selfplay 파일에서 c_puct를 실제 검색까지 전달한다. 기본 1.5 동작은 유지한다.
- [x] 작동하지 않는 queue/timeout을 새 설정에서 제거한다. 저장된 구형 값과 존재 여부는 체크포인트를 읽는 데 필요한 만큼만 보존한다. 새 migration/fallback을 만들지 않는다.
- [x] 이 무효 metadata의 값/존재 여부를 바꾸는 resume transition은 거부한다. 같은 저장 설정으로 재개하는 동작은 보존한다. 이는 기존 allowlist를 좁히는 의도적인 변경이다.
- [x] train_main.cpp의 네 군데 동일한 Trainer 설정 변환을 내부 함수 trainer_config(const ProductionConfig&) 하나로 합친다. 공개 헤더의 LibTorch 의존성은 늘리지 않는다.
- [x] 각 단계에서 다른 체크포인트 검증 조건을 큰 옵션 객체나 범용 wrapper로 합치지 않는다. 소스 선택과 검증 조건은 호출부에서 바로 보이게 둔다.
- [x] 실제 제품 CLI의 help/report와 배포 모델 validate를 확인했다. 사용자 중지 지시에 따라 학습 초기화·재개는 실행하지 않았다. 설정 전달과 재개 비교는 소스 검토·병렬 빌드로 확인했으며, 새 동작의 실행 검증을 했다고 주장하지 않는다.

## 2. 불필요한 경로 제거와 파일 설명

- [x] accepted만 반환하는 pipeline_main.cpp 실행 파일과 CMake 등록을 제거한다. 실제 사용하는 pipeline 함수는 유지한다.
- [x] 기존 보고서 접근 오류를 없는 보고서로 조용히 넘기지 않도록 직접 수정한다. alternate source/retry/default-object 경로를 만들지 않는다.
- [x] 파일별 표의 API와 구현에 책임, 실행 흐름, 단위, 소유권/수명, 동시성, 부작용, 실패와 호환성 이유를 상세히 설명한다.
- [x] arena의 실제 반복 정책과 root-owner/expert 선택을 명확히 설명한다. 현재 실험의 정책은 바꾸지 않는다.
- [x] 검토자는 주석과 구현이 일치하는지 확인한다. 불필요한 파일 분리나 새 계층을 추가하지 않는다.

## 3. 회귀 테스트와 GitHub CI 제거

- [x] regression-removal-notes.md의 참조 조사를 기준으로 기존/신규 회귀 테스트, 전용 fixture/golden, 테스트 전용 생성기를 삭제한다.
- [x] CTest/npm 테스트 등록, CI 테스트 실행 참조와 기존 GitHub CI workflows를 삭제한다. 새 테스트/CI나 대체 검사 프레임워크를 만들지 않는다.
- [x] SDK의 합성 모델 fixture 생성·포장은 테스트 전용이므로 제거한다. Desktop 실행에 실제 필요한 다섯 topology table은 보존한다. 회귀 자산을 다른 위치로 옮겨 숨겨 두지 않는다.
- [x] 남은 정상 빌드가 삭제된 테스트 자산을 요구하지 않도록 실제 소비자/CMake 참조를 정리한다. 제품 입력을 이름만 보고 삭제하지 않는다.
- [x] 현재 사용 문서의 테스트/CI 실행 안내를 정리한다. 과거 실험의 결과와 역사 문서는 소급 수정하지 않는다.

## 4. 빌드·실행 확인과 완료 기록

- [x] GCC/mold/sccache/Ninja로 제품과 필요한 벤치마크를 재구성·병렬 빌드한다. Dev는 O0/g1/pipe/noLTO, 실제 계산은 최적화 Release다.
- [x] 패널 production build와 실제 CLI의 필요한 실행만 확인한다. 테스트 suite는 다시 만들거나 실행하지 않는다.
- [x] git diff --check와 독립 소스 검토로 파일별 완료 상태를 기록한다.
- [x] graphify update .와 semantic 관계 검토 완료. 실제 제품 topology 입력 5개의 원본 해시를 보존했다.
- [x] 코드 정리·주석·테스트/CI 삭제·직접 실행 결과를 보고한다. 미완료 파일은 완료로 표시하지 않는다.

## 파일별 검토표

아래는 구현 후 독립 소스 검토를 마친 파일별 상태다. 주석은 책임·데이터 흐름·수명·동시성·부작용·정책을 설명한다. 실행 검증의 범위와 제한은 아래 기록을 따른다.

| 파일 | 정리/설명 핵심 | 동작 변경 | 상세 주석 |
|---|---|---|---|
| `native/include/diamond_orchestration/config.hpp` | 설정의 단위·소비자, 사용하지 않는 legacy 필드 | 구현·검토 완료 | 작성·검토 완료 |
| `native/src/config.cpp` | parse/validate/serialize, optional presence와 재개 비교 | 구현·검토 완료 | 작성·검토 완료 |
| `native/include/diamond_orchestration/training_wiring.hpp` | config→실행 설정의 변환 계약 | 구현·검토 완료 | 작성·검토 완료 |
| `native/src/training_wiring.cpp` | c_puct 전달, arena 고정 정책과 training 설정 차이 | 구현·검토 완료 | 작성·검토 완료 |
| `native/include/soo/selfplay.hpp` | episode, lane, worker, evaluator의 소유권과 수명 | 구현·검토 완료 | 작성·검토 완료 |
| `native/src/selfplay.cpp` | 배치 요청·검색 재개·취소와 오류 전달 | 구현·검토 완료 | 작성·검토 완료 |
| `native/src/train_main.cpp` | stage 흐름, 설정 중복, checkpoint 계약, 보고서 오류 | 구현·검토 완료 | 작성·검토 완료 |
| `native/src/pipeline_main.cpp` | accepted-only 옛 진입점의 삭제 및 참조 확인 | 삭제·참조 제거 완료 | accepted-only 기능을 제거 |
| `native/CMakeLists.txt` | 삭제 진입점의 build/test 관계 | 구현·검토 완료 | 작성·검토 완료 |
| `native/include/diamond_pipeline/pipeline.hpp` | pipeline primitives와 production coordinator의 역할 | 동작 유지 | 작성·검토 완료 |
| `native/src/pipeline.cpp` | self-play→sample→train, benchmark helper의 범위 | 동작 유지 | 작성·검토 완료 |
| `native/include/diamond_pipeline/replay_store.hpp` | open/sample/append의 부작용·수명·동시성 계약 | 동작 유지 | 작성·검토 완료 |
| `native/src/replay_store.cpp` | manifest/chunk 검증, orphan 정리, 샘플 선택 | 동작 유지 | 작성·검토 완료 |
| `native/include/diamond_pipeline/model_pool.hpp` | actor snapshot과 mutable staging buffer의 소유권 | 동작 유지 | 작성·검토 완료 |
| `native/src/model_pool.cpp` | install/activate/evaluate, precision과 timing의 의미 | 동작 유지 | 작성·검토 완료 |
| `native/include/diamond_training/trainer.hpp` | 모델·AdamW 상태와 학습 입력 계약 | 동작 유지 | 작성·검토 완료 |
| `native/src/trainer.cpp` | legal loss, GPU 동기화, 학습 실패와 metric 부재 구분 | 동작 유지 | 작성·검토 완료 |
| `native/include/diamond_training/checkpoint.hpp` | inspect/validate/load, weights와 exact resume 구분 | 동작 유지 | 작성·검토 완료 |
| `native/src/checkpoint.cpp` | 파일 검증·optimizer hash·transaction commit 순서 | 동작 유지 | 작성·검토 완료 |
| `native/include/soo/arena_router.hpp` | 검색 루트 소유자에 따른 후보/기준 모델 선택 | 동작 유지 | 작성·검토 완료 |
| `native/include/soo/min_experts.hpp` | 전문가 선택의 공개 입력 계약 | 동작 유지 | 작성·검토 완료 |
| `native/src/min_experts.cpp` | turn-order와 geometry에 따른 cyclic/reverse 선택 | 동작 유지 | 작성·검토 완료 |
| `native/benchmarks/min_expert_arena.cpp` | 고정 비교의 identity·실행·출력 경계 | 동작 유지 | 작성·검토 완료 |
| `native/src/schedule.cpp` | 실제 opening 생성, paired reuse와 freshness 조건 | 동작 유지 | 작성·검토 완료 |
| 기존 회귀 테스트·GitHub CI | 소스/전용 데이터/등록/실행 참조 제거 | 삭제·참조 제거 완료 | 사용자 요청으로 삭제 |

## 실제 확인 결과

- 네이티브 전체 개발 빌드: GCC 15/mold/sccache/Ninja, 병렬 6개, 23.94초. 개발 옵션 O0/g1/pipe, LTO 없음. core 병렬 12개 빌드도 통과.
- CPU Runtime SDK 빌드: 병렬 6개, 6.48초. 실제 topology 5개를 `data/topology/`로 이동하고 원본 해시를 보존. SDK 합성 모델 fixture 없음.
- 현재 패널이 사용하는 Release train/release 타깃: 병렬 6개, 23.08초. 고정된 실험용 바이너리는 변경하지 않음.
- 패널 production build와 HTTP 응답 정상. `/api/status`의 실행 작업은 0개. Release CLI help 및 기존 primary 학습 결과 report 정상. shipped Min 2.0.1 validate 정상.
- 회귀 테스트와 GitHub CI는 양쪽 저장소에서 삭제했으며 새로운 테스트·CI는 만들거나 실행하지 않음. 이미 수집한 실험 결과는 보존.
- 학습·평가는 사용자 요청으로 중지. 새 설정으로 훈련하거나 재개하지 않았으며, 미완료 confirmation family에 대한 성능 개선 결론은 내리지 않음.
- Desktop: 테스트 자산 없는 새 SDK를 별도 경로에 설치하고 정확한 manifest/archive lock으로 검증한 뒤 병렬 8개 빌드 통과, 8.78초. 제품 시작 경로와 SDK 검증은 독립 소스 검토 통과.
- Desktop 실제 실행: 격리된 설정 디렉터리와 offscreen Qt로 5초 정상 기동. 저장소의 기존 모델을 읽었으며 새 학습·대국은 실행하지 않음. 전체 GUI 수동 조작 검증은 아님.
- Desktop Graphify: 658 nodes/1,324 edges, QML semantic 18개와 관계 36개 보존, dangling edge 없음.
- Engine Graphify: 4,051 nodes/8,150 edges. 삭제된 소스의 노드와 dangling 관계 없음. 기존 제품 semantic 126개 보존, 현재 정리·중지 상태 4개 추가.

## YACHT 비교 후 추가 정리 (완료)

사용자가 추가 검토의 일곱 항목을 승인했다. 훈련·평가 중지와 자동 예약 삭제, 새 테스트·CI 금지, 로컬 브랜치 및 기존 아티팩트 보존 지시는 유지한다.

- [x] self-play 벤치마크에 production 설정을 먼저 적용하고 명시한 CLI 값만 덮어쓰기.
- [x] 기존 RunStage 정의에서 단계 이름·순서를 한 번만 관리.
- [x] 기존 checkpoint 환경 기록과 패널에 TF32 요청/허용 상태를 구분해 표시.
- [x] train_main의 단계별 실행을 같은 파일의 직접 호출 함수로 정리. 단계별 검증은 유지.
- [x] 호출자가 사용하지 않는 선택적 v2 저장 경로와 writer 삭제. end-to-end 벤치마크는 이미 명시적으로 v3를 저장하므로 그 경로를 유지.
- [x] 빌드 별칭·중복 컴파일 제거, Runtime 빌드에서 replay와 진단 실행파일 제외. 진단은 기존 BENCHMARKS 옵션으로 명시 선택.
- [x] 패널 목록/선택 작업 상세 분리, 과거 iteration 재독 감소, 로그 읽기 범위 제한과 전체 로그 접근 유지.
- [x] 독립 소스 검토, 병렬 빌드, 읽기 전용 CLI·패널 직접 확인, Graphify 갱신.


### 추가 정리 확인 결과

- `execute_stage`는 47줄의 직접 분기로 줄였고, 같은 파일의 단계별 함수 9개로 기존 본문을 옮겼다. 단계별 검사와 재개 판단은 유지했다.
- 설정/벤치마크, 네이티브 단계·저장·빌드, 패널의 독립 소스 검토에서 구체적 blocker는 발견되지 않았다.
- GCC 15/sccache/mold/Ninja: 수정 후 개발 빌드 병렬 6개 8.05초, Release train/release 병렬 4개 21.82초, Runtime 병렬 6개 0.15초(증분/캐시). 최초 개발 빌드의 누락 include 오류를 수정한 뒤 재빌드했다. 전체 클린 빌드 시간은 아니다.
- compile database에서 `training_sample.cpp`는 1회, Runtime의 replay/probe 소스는 0개임을 확인했다. Linux preset 이름과 datacenter 실행 스크립트의 옛 테스트 호출도 정리했다.
- 패널 Node 문법 및 Vite production build 통과(111ms), 재시작 후 홈 HTTP 200·없는 작업 HTTP 404·상태 jobs=[] 확인. 과거 iteration은 브라우저에 유지하고 최신 iteration부터 다시 읽으며, 콘솔 tail은 64 KiB로 제한한다. 전체 콘솔은 별도 스트림으로 제공한다.
- Release CLI help, 기존 primary run의 report, shipped Min 2.0.1 artifact validate가 정상 종료했다. selfplay benchmark는 help만 실행했고 실제 self-play·학습·평가는 실행하지 않았다.
- TF32 raw 환경 요청과 LibTorch 허용 flag를 새 checkpoint provenance에 기록한다. 실행 커널 사용의 증거는 아니다. 새 checkpoint를 만들기 위한 학습은 실행하지 않았다.
- Graphify AST 갱신 4,068 nodes/8,287 edges, 기존 semantic 130개 보존, dangling 관계 없음. 변경된 운영 문서의 의미 기록도 갱신했다.
- 회귀 테스트/CI를 추가하거나 실행하지 않았다. 실행 작업이 없으므로 살아 있는 작업의 incremental polling/full-log 다운로드는 소스 검토까지 확인했으며 실제 훈련으로 검증하지 않았다.
- 상세 로그: `artifacts/pipeline-cleanup-20261029/followup-verification/`. Desktop 코드/SDK lock과 shipped 모델, 실험 결과는 변경하지 않았다.

## 추가 감사 잔여 항목 정리 (완료)

사용자가 d04d0ed 재감사 항목의 실행을 승인했다. 학습·평가·자동메시지 중지, 테스트·CI 추가/실행 금지, 기존 데이터 보존 지시를 유지한다.

- [x] 제품 checkpoint/replay 코드의 실패 주입 6개와 전용 helper 제거.
- [x] checkpoint_every_iterations 지원값을 실제 동작과 같은 1로 제한.
- [x] self-play는 optimizer 없이 모델 가중치만 복원; 기존 context/integrity 검증 유지.
- [x] exact-resume 로더 내부의 중복 inspect/hash를 private helper로 통합.
- [x] 두 진단 구현/헤더를 benchmarks에 두고 해당 실행파일에만 컴파일.
- [x] 현재 프리셋과 실험 설정 디렉터리를 구분하고 실제 사용 경로 갱신.
- [x] 동기화 및 export/validate/promote 문서를 실제 명령과 일치.
- [x] 독립 소스 검토, 병렬 빌드·읽기 전용 CLI·패널 확인, Graphify 갱신.

Ruling: train_main의 추가 물리적 파일 분할은 이번 기능 정리 범위에 넣지 않는다. 직전 단계별 함수 분리는 유지하고, 새 레이어보다 실제 불필요한 실행 경로 제거에 집중한다.


### 잔여 항목 실제 확인 결과

- 실패 주입 checkpoint 1개/replay 5개와 전용 helper가 제품 소스에서 제거됨. 실제 write/flush/rename/rollback/cleanup 오류 처리는 유지했으며 독립 검토 통과.
- `checkpoint_every_iterations`는 `1`만 허용한다. 기존 필드/직렬화 형식은 유지한다. 과거에 1보다 큰 값을 쓰던 config/resume는 이제 거부될 수 있다. 이동한 17개 설정은 모두 1이며 원본과 바이트 단위로 동일하다.
- Self-play는 로컬 모델에 weights-only reader로 한 generation을 검사/로드한 뒤 context/lineage/config/protocol/topology를 확인하고 pool에 설치한다. AdamW는 복원하지 않는다. 무결성 확인을 위한 optimizer 파일 해싱은 유지한다.
- exact-resume의 v3→v2 내부 중복 검사는 private helper로 제거했다. 학습·후보 저장 호출부의 명시적 사전 context 검사는 유지하므로 해당 경로 전체에는 여전히 검사 2회가 있을 수 있다. 모든 checkpoint 읽기를 무조건 1회로 줄였다는 주장은 하지 않는다.
- 진단 구현과 헤더 4개는 `native/benchmarks/`로 이동. compile database에서 opt-in 진단 실행파일만 소유하며, 일반 Release 학습 빌드에는 두 helper의 컴파일 항목이 없다.
- 현재 프리셋 6개/보존 실험 11개를 분리. 실제 패널 API는 현재 프리셋 6개만 반환. 활성 실행문서/스크립트 경로 갱신, 과거 날짜별 보고서의 증거 경로는 보존했다.
- GCC 15/sccache/mold/Ninja 개발 빌드 병렬 6개 22.36초, Release train/checkpoint/release 병렬 4개 23.26초. 캐시를 사용한 증분 빌드이며 전체 클린 빌드 시간은 아니다.
- 패널 Node 문법/Vite build 통과(118ms), 서비스 재시작 후 HTTP 200·jobs=[]·presets=6 확인.
- Release CLI help/report 정상, primary iteration11/1536-step 기존 checkpoint validate 정상, shipped Min 2.0.1 validate 정상. 학습·평가·벤치마크·회귀 테스트를 실행하지 않았다. 새 self-play actor 경로와 optimizer exact-restore의 실제 실행 확인은 하지 않았으며 빌드·독립 소스 검토 범위다.
- Graphify 갱신: 기존 semantic 130개 유지, 옮긴 진단 파일로 문서 관계 2개 재연결, dangling 관계 없음. 운영 문서 의미 기록 갱신.
- 근거 로그: `artifacts/pipeline-cleanup-20261029/residual-verification/`. 기존 모델·실험 산출물·Desktop 저장소는 변경하지 않았고 자동 메시지는 재생성하지 않았다.

## YACHT 재대조: 미사용 API와 설정 의미 정리 (완료)

사용자가 잔여 미사용 코드 제거, 추가 스캔, 친절한 주석을 승인했다. 기존 C++ CLI의 직접 호출 구조를 유지한다. 새 Lua/Python launcher, 계층, 테스트, CI를 추가하지 않는다. 학습/평가/벤치마크 실행과 자동 예약은 중지 상태를 유지한다.

- [x] 호출자 없는 옛 arena 구현, rating 실패 주입 훅, ModelPool/Coordinator/RunState 편의 API 제거. 실제 I/O 오류 처리와 Runtime SDK 소비자는 보존.
- [x] seed 설정이 production과 benchmark에서 어디에 쓰이는지 명확히 하고 무효 설정의 노출을 정리. 과거 resolved config의 직렬화/해시/재현성과 run_seed 파생 규칙 유지.
- [x] native·도구·패널의 추가 미사용 코드/의존성을 검색해 확인된 내부 잔재만 정리. 공개 SDK와 호환 reader를 호출자 수만으로 삭제하지 않음.
- [x] 변경 파일의 역할·데이터 흐름·소유권·부작용·호환성에 설명 주석 추가; 단순 문장 반복 주석은 피함.
- [x] 독립 소스 검토, GCC/mold/sccache/Ninja 병렬 빌드, 직접 CLI/패널 읽기 확인, Graphify 갱신.

YACHT 비교 기준은 단일 책임 함수의 직접 연결, 명확한 명령/설정 계약, 실제 호출되는 모듈만 유지하는 것이다. 단순히 파일 수나 언어를 따라하지 않는다. 확인된 추가 후보와 처리 결과는 아래에 기록한다.

### 재대조 결과와 확인 범위

- 호출자 없는 `arena.cpp/arena.hpp`와 빌드 항목 삭제. 현재 production 평가와 `min_expert_arena`는 기존 경로를 유지한다.
- Rating 저장소의 테스트 전용 실패 주입 enum/콜백/setter/주입 2곳 삭제. 실제 파일 생성·flush·rename·충돌 검증은 유지한다. SDK 공개 헤더에서 테스트 API가 제거되며 Engine/Desktop 제품 호출자는 없었다. 설치된 SDK와 Desktop lock은 변경하지 않았다.
- `ModelPool`의 `install_checkpoint`, `resident_count`, `active_model`, `last_evaluation_stats`, `evaluated_batches`와 전용 상태/갱신 코드 제거. 벤치마크가 쓰는 누적 통계/reset은 유지. `Trainer::candidate_snapshot`와 사용되지 않는 candidate tensor role, `Coordinator::resume`, `RunState::with_progress` 제거. 실제 CLI resume/가중치 로드/actor·learner 분리는 유지한다.
- 새로운 현재 프리셋 6개에서 `mcts.seed`/`self_play.seed`만 제거. native 파서는 이를 선택적 legacy metadata로 읽고 과거 값과 존재 여부를 직렬화/동등성에 보존한다. 기존 checkpoint identity 및 run_seed 파생 규칙은 바꾸지 않는다. 옛 설정에서 legacy 필드를 지워서 resume하지 말고 저장된 config를 사용한다.
- standalone selfplay benchmark의 config base seed는 run_seed로 통일, 명시적 `--seed` 우선. 과거 서로 다른 self_play.seed로 benchmark를 돌렸다면 그 값을 `--seed`로 지정해야 한다. 기록된 실험 설정 11개와 산출물은 보존했다.
- 스크립트의 자체 테스트 진입점 2개와 build_resources의 test 모드/가짜 proc·cgroup 경로를 제거. 실제 연구 명령, 호스트 자원 제한, 플랫폼 지원은 유지한다. rating-sync의 중복 install을 SDK Runtime 규칙 하나로 정리했다.
- 현재 문서에서 opt-in benchmark 빌드, rating dry-run의 실제 네트워크/로컬 쓰기 동작, 역사적 Python checkpoint schema와 native-v3 계약의 차이를 명확히 했다. 과거 fixture manifest는 결과의 경로/해시 증거여서 보존한다.
- YACHT의 직접 명령 분기 구조를 참고했지만 새 launcher/언어/의존성은 추가하지 않았다. 패널 npm 의존성 3개는 실제 사용하는 것으로 확인했다. 호환 reader와 외부 소비자가 가능한 Runtime SDK API는 검색상 호출자가 적다는 이유로 삭제하지 않았다.

확인: GCC15/mold/sccache/Ninja 개발 -j8 21.23초, Release -j8 18.09초, Runtime -j4 0.14초 (캐시/증분 빌드). Native/도구/문서 독립 소스 검토 통과. CLI help/report, primary iteration11 checkpoint validate, shipped Min2.0.1 artifact validate 정상. Panel HTTP200/jobs=[]/현재 프리셋6개 확인. Shell/Node/Python 문법 확인. 새 회귀 테스트/CI나 학습·평가·benchmark workload는 실행하지 않았다. 설정의 과거 값/존재 보존은 코드 검토 범위이며 실제 resume를 실행하지 않았다.

Graphify: 4,015 nodes/8,180 edges, 기존 semantic 130개 보존, dangling 없음. 삭제한 옛 arena 파일로 향하던 역사 문서 관계 2개는 파일 삭제에 따라 제거했다. 현재 문서의 의미 기록을 함께 갱신한다. 검증 로그는 `artifacts/pipeline-cleanup-20261029/dead-code-verification/`에 보존한다.
