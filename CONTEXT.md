# Baddie

Baddie is a social media app for people of all genders to share posts, discover people and join conversations. Shopping is an optional feature within the app.

## Language

### Core Surfaces

**You Tab**:
The personal space for a person's profile, published posts, settings, rewards and installed mini apps. Shopping-related account tools also live here.
_Avoid_: Profile tab, account page, user center

**Explore Tab**:
The discovery tab for posts, people, communities, events, movies, games and bounties, alongside an optional Shop area.
_Avoid_: Shop tab, catalog page, marketplace feed

**Baddie Tab**:
The social content tab for watching media, reading Tea posts and joining conversations. Posts can include optional product links.
_Avoid_: Video tab, reels feed, entertainment tab

**You Apps Store**:
The only discovery surface for uninstalled mini apps, located inside the You tab apps area.
_Avoid_: App marketplace (global), mini app carousel, discovery strip

### Aesthetic & Content

**Aesthetic Room**:
The active visual theme context (`pink`, `vanilla`, `dreamy`, `midnight`) that controls accent styling and mood across supported UI.
_Avoid_: Theme mode, skin, palette profile

**Vibe**:
A style label used to classify content and packs for discovery; `All` is a filter value, not a style itself.
_Avoid_: Aesthetic room, theme, genre

**Baddie Post**:
A person's published content, including text, photos or video, with social interactions such as replies, likes and sharing. Product links and packs are optional attachments.
_Avoid_: Reel, story, clip

### Identity, Social, and Sharing

**User**:
The person using Baddie, whether they watch, read, post, follow people, join conversations or choose to shop or sell. These activities share one identity.
_Avoid_: Customer actor, creator actor, seller actor

**Brand**:
A shared record that people can create or select when posting. Brands never have login accounts. Everyone uses the same user account model; a brand may record who created it or omit that attribution when seeded.
_Avoid_: Seller role, post author, private brand tag

**Brand Attribution**:
The association between a post and its selected brand. The person who publishes remains the author; selecting a brand does not grant access to manage the brand record.
_Avoid_: Posting as the brand, transferring authorship

**Brand Creator**:
The person who originally adds a brand, when known. Creator attribution is optional and does not limit who may select that brand for a post.
_Avoid_: Exclusive poster, mandatory brand owner

**Creator Role**:
A user role for publishing content and interacting with an audience. Creating does not require selling products or attaching shopping links.
_Avoid_: Influencer (generic), poster, uploader

**Role Activation**:
Role behavior is activated implicitly by entering a flow rather than by toggling a global active-role mode.
_Avoid_: Role switch mode, persona toggle, global role selector

**Public Profile**:
A shareable user or brand identity view keyed by handle, using the You screen and showing public metadata and published posts. A brand profile can include posts attributed to it by different authors.
_Avoid_: Bio page, channel page, user card

**Share Modal**:
The in-app share surface used to preview payloads and trigger outbound share actions for every mobile share entry point, with no exceptions unless a platform blocker is explicitly documented.
_Avoid_: Native share sheet, quick share popup, OS share menu

**Share Payload**:
The canonical share data contract (context, message, URL, preview metadata) produced by shared payload builders.
_Avoid_: Ad-hoc share object, inline message blob, manual share text

### Bestie spaces

**Companion**:
A user's named Bestie character, with its own appearance, care needs and learned skills.
_Avoid_: Account avatar, shared character

**Personal Room**:
A user's saved 3D space with furniture, lighting and decor. It is separate from an Aesthetic Room, which is an app theme.
_Avoid_: Aesthetic room, theme

**Home Visit**:
An invited guest's access to a specific owner's saved Personal Room. A visit does not grant furniture-editing rights.
_Avoid_: Shared-call copy, collaborative edit

**Shared-Call Room**:
A room copy used by participants in a call. Changes to the copy are separate from the owner's saved Personal Room.
_Avoid_: Home visit, owner's saved room

**Care Activity**:
A room activity through which a user looks after their companion's hunger, energy, cleanliness or enjoyment.
_Avoid_: Notification, account health

**Companion Skill**:
A lasting ability belonging to a specific Companion, improved through completed activities. Cooking, fighting, self-defence, charisma, fitness, hustle, dance and flirting are separate skills from care needs and street reputation.
_Avoid_: Care need, street reputation, account level

**Companion Task**:
An instruction selected by the user for a specific Companion to carry out in the game. A task is separate from a mission objective or an activity the Companion chooses for itself.
_Avoid_: Background job, quest, autonomous intent

**Companion Task Queue**:
A Companion's ordered list of tasks selected by its user, with at most one task in progress. Each saved Companion has its own queue.
_Avoid_: Offline Mutation Queue, care routine, mission list

**Blocked Companion Task**:
A queued instruction that cannot proceed because its destination, object, resources or permission are unavailable. It remains unfinished rather than counting as a successful activity.
_Avoid_: Completed task, skipped task

### Extensibility & Messaging

**Mini App**:
An installable extension package with a manifest, capabilities, and optional UI slot contributions.
_Avoid_: Plugin, widget pack, micro-feature

**Mini App Install**:
A user-level installation record for a mini app, including enablement and granted capabilities.
_Avoid_: Subscription, entitlement only, app download

**Mini App Capability**:
A declared permission boundary that controls what a mini app may access or perform in host context.
_Avoid_: Scope flag, feature toggle, runtime trick

**UI Slot Contribution**:
A mini app-provided UI insertion for an approved host slot.
_Avoid_: Arbitrary injection, global overlay, floating module

**Contextual Pill**:
A compact action prompt anchored to a screen context, optionally contributed by mini apps.
_Avoid_: Toast, snackbar, banner ad

### Release Communication

**Drop**:
One released app/build/update unit represented in user-facing release communication.
_Avoid_: Patch note batch, deployment event, sprint output

**What's New**:
The in-app release communication surface that explains user-impact changes for the current Drop.
_Avoid_: Changelog file, release log, internal notes

### Offline & Cache Operations

**Local-First Snapshot**:
A device-cached copy of a mobile surface payload that is shown immediately and reused when network requests fail.
_Avoid_: Backup payload, temp state, mirror object

**Stale-Offline View**:
The offline behavior where an expired snapshot is still shown while the device is disconnected, instead of blocking the screen.
_Avoid_: Hard offline error, blank-state fallback, forced reload mode

**Global Cache Burst**:
An operator-triggered invalidation event that tells the mobile app to refresh one or more cache domains.
_Avoid_: Manual refresh ping, random cache clear, admin notice hack

**Cache Burst Domain**:
The canonical domain label used by burst events (`cart`, `orders`, `explore`, `you`, `baddie`, `support`, `inbox`, `all`).
_Avoid_: Feature flag bucket, endpoint key, screen id alias

**Burst Transport Contract**:
The server-to-mobile delivery contract for cache bursts, piggybacked through Explore payload responses in the current architecture.
_Avoid_: Push-only invalidation, hidden polling side-channel, ad-hoc response flags

**Offline Mutation Queue**:
A temporary in-memory list of unsent user mutations that are retried after connectivity returns.
_Avoid_: Durable event log, permanent outbox, background job ledger

**Reconnect Conflict Policy**:
The rule that server state is authoritative when queued offline mutations and latest backend state diverge.
_Avoid_: Client wins merge, timestamp-only arbitration, silent local override

**Cache Kill Switch**:
An emergency runtime control that can disable cache writes or burst handling to reduce production risk during incidents.
_Avoid_: Permanent feature removal, debug-only toggle, release rollback substitute
