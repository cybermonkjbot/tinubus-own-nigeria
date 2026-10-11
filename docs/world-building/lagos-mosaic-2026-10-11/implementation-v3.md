# Route-aware activity authoring

October 11, 2026. We added offline movement intent and bounded logical service queues to the existing Yaba authoring slice. This patch introduces no new geographic or cultural claims and uses the existing researched layout inputs.

## Delivered

`mosaic_activity.py` supplies an explicit-clock checkpoint and one-minute stepping API. Callers must supply initial node positions; the default packet deliberately supplies none. Missing positions remain `entry_required`, not inferred street arrivals. Movement interpolates along available authored walking edges, retains an exact edge cursor during blockage, and replans from the next reached node when an intention changes. Disconnected destinations remain unavailable without a position jump. Ending an activity cancels its old request; an authoring despawn marker clears the offline position.

Queues use deterministic arrival-minute/actor-ID ordering, separate service and waiting limits, a fictional three-minute service duration, completion tracking, capacity rejection/retry and explicit caller cancellation. They contain existing positioned routine actors only. Cancellation lasts through the current activity. Event demand without positioned arrivals is reported separately and does not consume queue slots or spawn people. Queue ranks are logical: no pavement waiting positions, collision clearance or usable interior is established.

`mosaic_export.py` includes the activity policy, default checkpoint and module hash in its immutable packet. `mosaic_native.py` includes the same data in its future receipt. Neither path activates runtime NPC behavior. Existing traffic, resident and utility authority remain unchanged.

The generated local packet is `output/lagos-mosaic-authoring/mosaic-yaba-c30590f282dbf1fed146c2f6f1c7eacbde9c546c9427d666756c60e85d30a7c2.json`, SHA-256 `c30590f282dbf1fed146c2f6f1c7eacbde9c546c9427d666756c60e85d30a7c2`. Generation reported 3,108 placement records and 29 destination records. Earlier immutable packets are retained; generated packets remain outside this source checkpoint.

## Integration Boundary

The stepping API accepts exactly the next game minute. Seeking, rewinding or changing actor identities requires an explicit new checkpoint. Walking speed and seconds-per-game-minute are authored policy values, not measured Lagos behavior. Caller blocks apply to the sampled step; interval-level obstruction timing and continuous collision remain future runtime work. Initial positions and routes are authoring inputs, not native traversal admission.

Source and diff review are the verification method for this patch. No tests, lint, typechecks, compilation, application builds or additional preview processes are authorized or run. Native map creation, rendered inspection and live gameplay remain pending. Production renderer registration and mobile behavior are untouched.
