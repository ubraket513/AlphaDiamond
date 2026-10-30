# Min turn-order experts experiment

The user requested two independent Min networks for the previously diagnosed
turn-order groups, correct routing, and comparison against the current shared
model. The frozen reference is the promoted shared checkpoint at local step
128, model Min 2.1.0 with goals-v2 encoding. Its immutable artifact is copied to
`artifacts/min-experts-20261014/baseline/shared-step128`.

The two experts each own every parameter, AdamW state, and replay store. Their
initial weights are byte-identical to the reference. Each receives only its
three supported orders: cyclic is 123/231/312 and reverse is 132/213/321 in
the standard board's physical IDs. Membership is derived from the standard
match factory's camp/goal geometry and full cyclic order, not numeric player
IDs, current actor, or finish status. Unsupported geometry fails explicitly.

Training uses the existing durable trainer with an optional immutable
`self_play.turn_order_group` setting (`all`, `cyclic`, `reverse`). Default `all`
is omitted from serialized configurations to preserve historical identities.
Specialists require Min and balanced orders. The tensor/rules Compatibility
contract remains unchanged; resolved-config/run-identity validation and
separate replay roots isolate normal continuation. Manually copying replay
between run directories is outside this workflow and is not an import feature.

An expert evaluator routes using the actual request's full Match, propagated
through BatchItem. The existing arena ownership router remains outermost:
first the actual search root chooses candidate versus opponent, then the
candidate bundle chooses its specialist. A simulated leaf's actor never
changes model ownership or the game's expert. Both prepare and evaluate use
the same routing rule. No shared trunk or learned gating is introduced.

Initial pilot: 768 self-play games and 256 optimizer updates per expert,
across two iterations, batch 512, learning rate 0.00002, 512 search simulations,
fresh optimizers/replay, goals-v2 inputs. The user selected a matched
continued-shared control, receiving the combined 1536 games/512 updates.
All start from the same frozen reference. Shared-pilot work
already in flight is retained and stopped at a durable stage before new GPU
training starts.

A native expert arena evaluates the routed bundle against two copies of the
frozen reference over 12 new opening blocks, 432 games, balanced across all
orders and candidate seats. Every specialist therefore gets 216 games in its
own domain. The shared control uses identical opening/seat schedules and
seeds. Save individual completed/aborted outcomes, checkpoint/config hashes,
and immutable arm-specific reports. Report both groups, all six orders, and
opening-block uncertainty using JAX. No deployment or automatic claim of
superiority follows from training losses or aggregate score alone.

Tests cover routing under actor/finish changes, rotations and ID relabeling,
mixed batches and lane reuse, correct candidate ownership, configuration
round trips and rejection, isolated warm starts/resume, and CPU end-to-end
arena execution. Existing fixtures and shipped models remain unchanged.
