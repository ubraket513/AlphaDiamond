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
