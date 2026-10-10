# Baddie

## Domains

- Baddie's primary website and email domain is `baddieapp.co.uk`. Use it by default for public URLs, email and DNS work.
- `baddieapp.com` and `baddieapp.ng` are additional managed domains. Confirm the target service's configuration before changing their DNS.
- `baddie.ng` is the legacy repository directory name, not the production domain.

## Product

- Baddie is a social media app centred on people, posts, expression and conversation. Shopping is an optional feature within that social experience.
- Baddie's audience includes people of all genders. Use gender-inclusive product language and define audience segments by interests, participation or the relevant feature. Brand colours, historical personas and individual campaigns do not define the platform audience.
- Core mobile tabs are You, Explore and Baddie. Reuse the existing navigation, platform services and feature patterns.
- Use `CONTEXT.md` for product language. Read `docs/product/shopping-language.md` for shopping, seller, payment or order work. Historical shopping plans do not define the app's identity, audience or priorities.
- Keep changes focused on current needs. Avoid speculative systems and duplicated implementations.
- Preserve unrelated work and checkpoint substantial changes in git.

## Implementation priority

- Focus on implementing the requested change. Write, update or run tests only when the user explicitly asks for test work.
- Except for required companion renderer packaging below, run typechecks, lint, builds, compilation, bundling, app exports and other resource-intensive validation only when the user explicitly requests that operation. This includes targeted runs, watch modes, background jobs, delegated agents and scripts that invoke these operations indirectly. Never substitute one validation command for another to bypass this rule.
- A request to build a feature means implement it, not run a build command. General instructions to verify, finish or prepare work do not authorize these operations. Defer them to the user's explicitly requested verification or deployment pass.
- Verify implementation through source review, diff inspection and inspection of already-running previews. Reuse existing servers; do not start extra servers, simulators or build processes solely for verification. If rendered inspection requires an unrequested process, leave it deferred. Leave existing tests intact and defer test coverage until requested.
- This opt-in rule applies across the repository, including checks recommended by skills or nested instructions. Before running a script, inspect what it invokes; run only operations authorized for the current task.

## Companion renderer updates

- Whenever a requested implementation changes production companion renderer code, consumed assets or shared scene/map data, asset registry entries or bridge contracts, regenerate and register matching native and web renderer documents in the same patch. Renderer updates are part of implementation, not an optional later step.
- This is standing authorization for the required renderer packaging only. Tests, typechecks, lint, full app builds, app exports and releases still require explicit requests. Read-only audits and documentation-only changes do not trigger packaging.
- Use the existing packaging scripts in `packages/companion-scene/package.json`. Retain content-addressed HTML and JSON outputs in `apps/mobile/assets/companion-scene` and update both `bundledCompanionSceneRuntime.ts` and `bundledCompanionSceneRuntime.web.ts` registrations. Preserve earlier immutable documents.
- Confirm both registered documents match the current shared protocol and complete asset contract, including changed asset paths and hashes. Inspect an already-running game preview when available; report in-game verification and release status separately from packaging and R2 delivery.
- Keep renderer-affecting work incomplete if packaging or registration fails, or if the user explicitly defers the renderer update. Source integration or an R2 upload alone does not establish app use.

## Communication

- Treat the user, operators and readers as competent adults. Give the requested result concisely; explain further only when asked or when a decision requires it.
- Do not explain obvious UI, labels, buttons, charts or routine controls. On admin pages, prefer direct labels, numbers and decisions over tutorial copy or narrative guidance.
- Add disclosures only when explicitly requested or required for consent, safety, legal obligations or factual accuracy. Omit unsolicited caveats, irrelevant notes, implementation commentary and explanations of familiar controls.
- Baddie is a live product. Describe verified current behavior; use prototype, demo or pre-launch labels only for work that actually has that status.

## Copy and author voice

- Apply these rules to all copy: product UI, websites, decks, documents, scripts, captions, presenter notes and exports.
- Write in the intended author's voice. For Baddie's own narrative, use "we" and "our". Use "I" for the author's personal experience, with an explicit transition when moving between personal and company statements.
- Keep the speaker and audience consistent throughout each passage. Use "you" only when deliberately addressing the reader; describe people, creators and brands in the third person when discussing whom we serve. Use shopper, buyer or seller language only for the relevant shopping action.
- Own our statements directly: "We've invested", "We're hearing interest", "We will measure". Avoid outside-reviewer labels such as "founder-reported", "our founder reports" or "founder thesis" in our own copy.
- Preserve factual distinctions with dates, sources and clear labels for plans, assumptions and targets. Do not turn an estimate into a result or repeat defensive qualifications in place of a clear statement.
- Label projections once per section. Keep material inputs, dates and funding conditions together instead of repeating forecast disclaimers.
- Never mention hypotheses to investors in app screens, websites, decks, scripts, shared presenter notes, data-room documents, exports or messages. Keep untested ideas and speculative claims in separate internal working material. Use verified facts, supported product descriptions, approved plans and clearly labeled targets or projections; do not disguise unsupported claims as facts or omit material forecast inputs and funding conditions.
- Before finishing, read every changed passage and its surrounding copy for unintended shifts in person, reviewer commentary and instructions accidentally addressed to the author. Check the same voice in summaries, notes and rendered documents.

## What's New

- Update `apps/mobile/src/features/you/whatsNewDropContent.ts` before test/preview/production builds and `eas update` publishes.
- Mobile changes and shared changes affecting mobile behavior, runtime or releases require a content update in the same patch. Run the What's New guard before finishing.
- Keep at most five current highlights. Replace old notes; retain history in git.
- Write for people using Baddie: what they can do or what problem is fixed. One topic and one or two short sentences per slide.
- Verify claims, state relevant platform limits, and keep visuals and metadata consistent with the slides.
- Exclude technical internals, admin/investor updates, vague polish claims and unsupported promises.

## Brand and UI

- CEO posts and CEO-authored emails use Joshua's approved photograph at `apps/landing/src/assets/investor/team/joshua-anop.webp` in our flower avatar, reusing the geometry in `apps/mobile/src/features/you/components/FlowerProfileAvatar.tsx`.
- Emails retain a distinct header and footer. Keep the header free of murals and the reading area white, with brand pink accents and white text on pink fills. Use murals within the body or as a quiet outer background. Keep footer support, unsubscribe and legal details without repeating the brand name or website.
- For newsletter design, layout or artwork changes, read the approved newsletter design standard in `docs/email/agent-newsletters.md`.
- No emojis in code, UI, responses or documentation.
- No decorative eyebrow headings or filler copy on admin pages. Text must help an operator understand data, state or an action.
- Match the current You page's neutral glass borders. Never use solid pink, red or accent outlines; use fills for selection.
- Present every action confirmation in a dedicated sheet or modal appropriate to the action and platform. Never embed confirmation prompts in cards, lists, forms or page content, or turn an action button into an inline second-tap confirmation. Reuse the existing overlay components and preserve cancellation, busy states and error recovery.
- Edit saved items, including addresses, profiles, posts and admin records, in dedicated platform-appropriate sheets or modals across all apps. Keep new-item forms and quick controls such as quantity adjustments and toggles in place. Seed each editor from the saved record, discard draft changes on cancel, and keep failed saves open with actionable errors.
- Explore styling follows Pink, Vanilla, Dreamy or Midnight. Cart interactions stay in a bottom sheet, with existing animation and haptic patterns.
- Full-screen media containers extend behind system bars. Only readable/tappable overlays use safe-area offsets. Preserve existing contain/crop behavior.
- Before adding tooltips, introductory coachmarks or contextual field help, read `docs/design-system/contextual-help.md`. Keep essential information inline and introductions optional.

## UI regression protection

- Evaluate shared prerequisites at the view level before mounting grids, split panels or sibling cards. When a shared prerequisite is missing, render one parent-level empty state with one primary action.
- Keep genuinely empty sections beside populated content to muted inline text. Use full empty states only for the view, without nested illustrations or competing setup actions.
- Review empty, loading and error states separately. Preserve populated or stale data and actionable errors; missing data is not a confirmed zero.

- For UI changes, including shared styles, tokens and data changes that affect rendering, identify affected screens and shared-component consumers before editing. Preserve existing appearance and interactions outside the requested change.
- Capture the affected UI before editing and compare it with the rendered result using the same viewport, theme and data state. Reuse existing visual checks and baselines. Refresh a baseline only after verifying that each difference belongs to the requested change, never just to clear a failing check.
- Verify affected layouts at narrow and wide widths and on the relevant native and web platforms. Check applicable themes, text scaling, safe areas, media crop, scrolling, keyboard behavior and overlays. Exercise changed interactions and relevant loading, empty, error and populated states.
- Use direct rendered inspection for visual defects; source assertions, type checks and builds alone do not establish visual correctness. When the user requests regression tests for a bug fix, confirm they fail on the original defect and pass with the fix.
- Before completion, inspect the rendered UI for unintended differences. Run checks only within the implementation priority rules above. Fix regressions introduced by the change. If a platform or state cannot be verified, identify the gap explicitly and leave UI verification incomplete rather than claiming a pass.

## Very important: respect the audience's competence

- Apply this to product screens, flows, marketing, videos, scripts, captions and exports. Treat viewers and users as competent adults; let clear headlines, visuals and direct actions do the work.
- Do not add labels such as "Illustrative notification" or "For illustration only" to marketing visuals by default. Remove patronizing explanations and text that merely describes what viewers can already see. Keep production context in internal notes; retain only disclosures required under Communication above.
- Remove copy that repeats a heading, field, button or status. Explain unfamiliar behavior and material constraints, not how ordinary forms, switches, photo pickers or saved drafts work.
- Use adult-to-adult language. Avoid patronizing nicknames, permission-giving reassurance and motivational filler around routine tasks.
- Keep routine actions direct. Do not add mandatory tab tours, acknowledgement-only screens, extra completion taps, artificial waits or simulated progress for ceremony. Show progress only for actual work; keep demos explicit and optional.
- Preserve accessible names and useful screen-reader guidance, input requirements, fees, consent, privacy consequences, destructive-action safeguards and actionable error recovery. Brevity must not hide information needed for a decision.
- Before finishing any copy or UI change, scan the changed material and surrounding flow for over-explanation, repeated labels, unsolicited disclosures and irrelevant notes. Remove any sentence, screen, pause or tap whose absence would leave the task equally clear and safe. Prefer removing unnecessary steps over merely shortening their copy.

## Mini apps

- Uninstalled mini apps are discoverable only in You's apps store. Remove paths that enable discovery or install promotions elsewhere, including from installed-app extensions.

## Admin data

- Use centralized metric APIs. Show units, comparison periods and data freshness; distinguish missing or stale data from zero.
- Every widget must support an operator decision. Keep filters, metric definitions and table/report exports accessible.
- For social analytics, treat audience and creator activity as the core business asset. Default to metrics that show account value, content performance, retention, monetization rate and revenue per active account, with commerce as one channel rather than the whole business.
- Do not label a commerce metric as total business revenue. Separate commerce, services, advertising, meet and other revenue streams unless the metric actually combines them.
- Use readable charts, keyboard-operable controls and text labels alongside status colors.

## Research and completion

- Use repository context first. Research externally only for uncertain/current APIs, integrations or design decisions that need validation.
- Check installed versions before following library documentation.
- Fix violations in edited code and review the diff before declaring completion. Run checks only within the implementation priority rules above.
