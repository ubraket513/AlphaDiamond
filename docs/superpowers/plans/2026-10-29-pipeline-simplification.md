# Pipeline Simplification and File Commentary Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 기존 학습 파이프라인을 단순화하고, 사용자가 파일별로 직접 검토할 수 있는 상세한 소스 설명을 제공한다.

**Architecture:** 기존 config, training_wiring, train_main, pipeline, replay, model, trainer 모듈을 유지하며 중복과 무효 경로를 제거한다. 새 실행 계층이나 호환성 프레임워크를 만들지 않는다.

**Tech Stack:** C++/LibTorch, CMake/Ninja, GCC/mold/sccache, CTest, Serena, Graphify.

**Spec:** `docs/superpowers/specs/2026-10-29-pipeline-simplification-design.md`

## Global Constraints

- 사용자 승인된 실행 방식: 로컬 브랜치, 독립 작업은 subagent 활용, 추가 승인 대기 없음.
- 여섯 confirmation arm의 정상 완료와 최종 분석 전에는 핵심 소스를 변경하지 않는다. root만 GPU 작업을 시작한다.
- 현재 실험의 frozen source/binary/config/model identity와 shipped 모델을 변경하지 않는다.
- 모든 변경 파일에 책임·계약·복잡한 정책을 설명하는 주석을 검토한다. 삭제 파일은 검토표에 삭제 이유와 참조 확인 결과를 남긴다.
- Dev는 O0/g1/pipe/noLTO, 실제 학습/평가는 최적화 Release. 병렬 빌드와 병렬 테스트 모두 사용한다.
- 마감 2026-10-29 16:20:02 KST. 그때 남은 코드 작업과 검증을 정확히 보고한다.

## Review Focus

- 구형 설정의 비기본 queue/timeout 값: 재직렬화와 체크포인트 재개 호환성을 잃지 않는다.
- 비기본 c_puct: self-play와 arena가 모두 요청 값을 사용한다. 기존 기본 1.5는 유지한다.
- 보고서가 없음/기존 파일을 열 수 없음: 미실행 단계와 접근 오류를 혼동하지 않는다.
- 구형 checkpoint/report: 호환 처리 통합이 identity 검증을 약화하지 않는다.
- Min 검색의 root owner/turn order: 후보와 기준 모델 또는 cyclic/reverse 전문가의 소유권을 바꾸지 않는다.

## Task 1: 설정에서 실제 검색까지 연결

**Files:** `native/include/diamond_orchestration/config.hpp`, `native/src/config.cpp`, `native/include/diamond_orchestration/training_wiring.hpp`, `native/src/training_wiring.cpp`, `native/include/soo/selfplay.hpp`, `native/src/selfplay.cpp`, 세 개 `configs/alphazero/min-expert-*-local-pilot.json`.

**Interfaces:** `ProductionConfig` → 기존 `wire_training_iteration`/`wire_arena_episode` → `EpisodeConfig` → 검색 설정. Trainer 설정 변환은 Task 2의 실행 파일 내부 함수가 담당한다; 설정 헤더에 LibTorch 의존성을 추가하지 않는다.

- [ ] 기존 `config_test`, `cli_contract_test`, `selfplay_test`의 관련 assertions를 확인하고 비기본 c_puct 전달과 invalid finite/range 입력의 회귀를 추가한다.
- [ ] 현재 코드에서 해당 회귀가 실패함을 확인한다.
- [ ] c_puct를 전달하고 쓰이지 않는 queue/timeout은 optional legacy data로만 보존한다. 기존 값이 있으면 정확히 보존하고 새 설정에는 생성하지 않는다.
- [ ] legacy 값의 roundtrip, 잘못된 타입 거부, 재개 시 값/존재 여부 변경 금지와 새 설정의 생략을 검증한다. 현재 resume allowlist에서 허용하던 이 두 무효 필드의 변경을 거부하는 것은 의도적인 동작 변경이다. 같은 저장 설정으로 재개하는 호환성은 유지한다.
- [ ] 각 파일에 단위, 실행 소비자, 배치/스레드/게임 수의 차이, 실제 arena 고정 정책과 호환성 계약을 설명한다.
- [ ] 필요한 타깃을 병렬 빌드한 뒤 관련 CTest를 `-j2` 이상으로 실행하고 결과를 기록한다.

## Task 2: 실행 경로와 중복 설정 정리

**Files:** `native/src/train_main.cpp`, `native/src/pipeline_main.cpp`(삭제), `native/CMakeLists.txt`, 참조가 확인된 현재 문서와 CLI 테스트.

**Interfaces:** `train_main.cpp` 내부 `diamond_training::TrainingConfig trainer_config(const ProductionConfig& config)` 함수로 네 군데 설정 변환을 통합한다. `load_iteration_source`/`load_actor_checkpoint`는 입력 계약이 같은 부분만 내부 helper로 합친다. 공개 CLI의 실제 학습 진입점은 유지한다.

- [ ] 기존 `cli_contract_test`의 실제 초기화·migration·resume 동작 검증을 확인한다. 외부에서 관측 가능한 loss/optimizer 계약의 공백이 있을 때만 해당 회귀 사례를 추가한다. 비공개 생성 지점마다 직접 관측 가능하다고 가정하지 않는다.
- [ ] 보고서 부재와 기존 보고서 접근 실패를 구분하는 회귀를 추가하고 실패를 확인한다. 접근 오류 테스트는 root 실행에서도 재현되는 fixture를 사용한다.
- [ ] 네 군데 설정 변환을 실행 파일 내부 공통 함수로 교체하고, 중복 검증 순서를 의미 보존 조건 아래 통합한다.
- [ ] accepted-only 실행 파일의 CMake/테스트/현행 문서 참조를 확인한 뒤 삭제한다. 실행 중인 제품이나 일반 pipeline primitives는 삭제하지 않는다.
- [ ] 단계별 입력·출력·저장 순서와 legacy 분기의 이유를 자세히 설명하고, 오류를 덮는 분기만 수정한다.
- [ ] `cli_contract_test`, `coordinator_resume_test`, `final_pipeline_smoke_test`, `native_pipeline_smoke_test`, `production_topology_test`, `runtime_boundary_test` 및 실제 영향 테스트를 병렬 실행한다.

## Task 3: 데이터와 실행 자원의 파일별 설명

**Files:** `native/include/diamond_pipeline/{pipeline,replay_store,model_pool}.hpp`, `native/src/{pipeline,replay_store,model_pool}.cpp`, `native/include/diamond_training/{trainer,checkpoint}.hpp`, `native/src/{trainer,checkpoint}.cpp`. 실제 파일 존재와 사용 관계를 Serena로 확인한다.

**Interfaces:** 공개 API와 저장 형식 변경 없음. 주석은 실제 소유권/부작용/동기화 계약을 설명한다.

- [ ] 파일마다 호출자·피호출자, 수명, 공유/독점 자원, 저장 형식과 오류 경계를 조사한다.
- [ ] replay open의 cleanup 부작용, checkpoint commit 순서와 identity, model pool의 install/activate와 평가 소유권, Trainer의 optimizer 복원과 CUDA 완료 시점/telemetry 제한을 설명한다.
- [ ] `pipeline::run_iteration`과 production coordinator의 실제 역할 차이를 명시한다. 사용 중인 테스트/벤치마크 경로를 추측으로 삭제하지 않는다.
- [ ] 독립 리뷰로 모든 설명을 소스와 대조한다. 주석만 변경했으면 동일 동작을 위한 새 테스트를 만들지 않는다.

## Task 4: 평가 선택 경로 설명과 통합 검토

**Files:** `native/include/soo/arena_router.hpp`, `native/include/soo/min_experts.hpp`, `native/src/min_experts.cpp`, `native/benchmarks/min_expert_arena.cpp`, `native/src/schedule.cpp`, 관련 수정 테스트, 이 문서의 파일별 검토표.

**Interfaces:** frozen 평가가 끝난 후에도 root ownership, turn-order 선택, opening schedule 생성 알고리즘과 검증 기준은 유지한다.

- [ ] 모델 routing이 실패 시 대체 실행이 아닌 소유권 선택임을 입력/출력과 예로 설명한다.
- [ ] seed/index에서 실제 opening이 생성되는 방식과 paired evaluation의 재사용 조건을 설명한다.
- [ ] 이 문서의 파일별 검토표에 역할, 제거/통합/주석 변경, 유지한 이유, 테스트 증거와 남은 일을 기록한다. 각 파일은 완료 전까지 미완료 상태로 둔다.
- [ ] 독립 whole-diff 리뷰, 필요한 병렬 CTest, `git diff --check`를 통과한다.
- [ ] `graphify update .` 후 semantic node/edge 보존과 dangling edge를 검사한다. 문서 의미 관계는 별도 확인한다.
- [ ] 변경한 파일만 묶어 커밋하고 사용자의 기존 변경/실험 산출물은 보존한다.

## 범위 밖

새 추상화 계층, 저장 형식 재작성, 전문가 통합 모델, 새 routing policy, 전면 디렉터리 재배치, dependency 교체, 자동 fallback은 이번 정리에 포함하지 않는다. unrelated 파일까지 읽지 않고 일괄 주석을 생성하여 검토 완료로 표시하지 않는다.

## 파일별 검토표

아래 상태는 계획 작성 시점의 실제 진행 상태다. `준비`는 소스를 조사했거나 정리안을 작성한 상태이며, 상세 주석이 이미 적용됐다는 뜻이 아니다. 동작 변경과 설명 검토를 따로 완료 표시한다.

| 파일 | 정리/설명 핵심 | 동작 변경 | 상세 주석 |
|---|---|---|---|
| `native/include/diamond_orchestration/config.hpp` | 설정의 단위·소비자, 사용하지 않는 legacy 필드 | 준비 | 준비 |
| `native/src/config.cpp` | parse/validate/serialize, optional presence와 재개 비교 | 준비 | 준비 |
| `native/include/diamond_orchestration/training_wiring.hpp` | config→실행 설정의 변환 계약 | 준비 | 준비 |
| `native/src/training_wiring.cpp` | c_puct 전달, arena 고정 정책과 training 설정 차이 | 준비 | 준비 |
| `native/include/soo/selfplay.hpp` | episode, lane, worker, evaluator의 소유권과 수명 | 준비 | 준비 |
| `native/src/selfplay.cpp` | 배치 요청·검색 재개·취소와 오류 전달 | 준비 | 준비 |
| `native/src/train_main.cpp` | stage 흐름, 설정 중복, checkpoint 계약, 보고서 오류 | 준비 | 준비 |
| `native/src/pipeline_main.cpp` | accepted-only 옛 진입점의 삭제 및 참조 확인 | 준비 | 삭제 이유 기록 예정 |
| `native/CMakeLists.txt` | 삭제 진입점의 build/test 관계 | 준비 | 준비 |
| `native/include/diamond_pipeline/pipeline.hpp` | pipeline primitives와 production coordinator의 역할 | 변경 필요성 검토 | 준비 |
| `native/src/pipeline.cpp` | self-play→sample→train, benchmark helper의 범위 | 변경 필요성 검토 | 준비 |
| `native/include/diamond_pipeline/replay_store.hpp` | open/sample/append의 부작용·수명·동시성 계약 | 변경 없음 | 준비 |
| `native/src/replay_store.cpp` | manifest/chunk 검증, orphan 정리, 샘플 선택 | 변경 없음 | 준비 |
| `native/include/diamond_pipeline/model_pool.hpp` | actor snapshot과 mutable staging buffer의 소유권 | 변경 없음 | 준비 |
| `native/src/model_pool.cpp` | install/activate/evaluate, precision과 timing의 의미 | 변경 없음 | 준비 |
| `native/include/diamond_training/trainer.hpp` | 모델·AdamW 상태와 학습 입력 계약 | 변경 없음 | 준비 |
| `native/src/trainer.cpp` | legal loss, GPU 동기화, 학습 실패와 metric 부재 구분 | 변경 없음 | 준비 |
| `native/include/diamond_training/checkpoint.hpp` | inspect/validate/load, weights와 exact resume 구분 | 변경 없음 | 준비 |
| `native/src/checkpoint.cpp` | 파일 검증·optimizer hash·transaction commit 순서 | 변경 없음 | 준비 |
| `native/include/soo/arena_router.hpp` | 검색 루트 소유자에 따른 후보/기준 모델 선택 | 유지 | 평가 완료 후 |
| `native/include/soo/min_experts.hpp` | 전문가 선택의 공개 입력 계약 | 유지 | 평가 완료 후 |
| `native/src/min_experts.cpp` | turn-order와 geometry에 따른 cyclic/reverse 선택 | 유지 | 평가 완료 후 |
| `native/benchmarks/min_expert_arena.cpp` | 고정 비교의 identity·실행·출력 경계 | 유지 | 평가 완료 후 |
| `native/src/schedule.cpp` | 실제 opening 생성, paired reuse와 freshness 조건 | 유지 | 평가 완료 후 |
| 수정한 테스트 파일 각각 | 재현 입력, 검증 계약, 실패 시 드러나는 회귀 | 해당 작업에서 기록 | 해당 작업에서 기록 |

주요 수정 후 이 표를 실제 커밋·테스트 결과로 갱신한다. 세부 설명의 독립 검토가 끝나지 않은 파일은 완료로 표시하지 않는다.

## Task 5: 최종 검증 후 기존·신규 회귀 테스트 전체 제거

**사용자 확정 범위:** 기존 것을 포함한 회귀 테스트 전체. 추가 승인 불필요. 이 Task는 앞의 정리·검증·리뷰가 끝난 후 실행한다.

**Files:** 실제 참조 조사 결과를 `regression-removal-notes.md`에 열거한다. `native/tests/`, 저장소 `tests/`의 회귀 전용 fixture/golden, panel `*.test.*`, 관련 CMake/package/CI 등록과 전용 생성 도구가 조사 대상이다. 공유 제품/벤치마크 자산은 이름만 보고 삭제하지 않는다.

- [ ] 기존 및 신규 회귀 테스트를 최종 코드에서 병렬 실행하고 명령·결과·시점·커밋을 기록한다.
- [ ] Serena 참조와 Graphify 관계로 테스트 전용 파일 및 공유 소비자를 확인해 정확한 삭제 목록을 확정한다.
- [ ] 회귀 테스트 소스와 전용 자산, CTest/npm 등록, CI의 해당 실행 참조를 함께 제거한다. 테스트를 새 위치로 숨겨 남기거나 다른 이름으로 대체하지 않는다.
- [ ] 테스트 삭제 후 남은 제품/벤치마크 타깃을 CMake 재구성·병렬 빌드하고, 패널 production build 및 실제 CLI의 최소 실행을 검증한다. 삭제된 테스트를 다시 만들지 않는다.
- [ ] 현재 사용 문서의 실행 명령을 정리하고 Graphify를 갱신한다. 삭제된 테스트에 대한 과거 실험 결과/역사 문서는 역사로 보존한다.
- [ ] 파일별 검토표와 최종 보고서에 검증 결과와 실제 삭제 범위를 기록한다. 앞선 '테스트 보존' 문구는 검증 전까지의 순서 조건으로 해석한다.
