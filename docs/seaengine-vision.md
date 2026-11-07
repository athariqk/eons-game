# SeaEngine — Foundational Vision & Philosophy

> "The universe compressed within the digital world under your fingertips. Quietly shaping. Inspiring you to become."

---

## The Core Idea

**SeaEngine** is an ocean exploration sandbox inspired by *SpaceEngine*, but grounded in Earth's oceans.

It is an attempt to create a digital space that captures the mysterious, wondrous, and majestic atmosphere of the deep sea and the surface ocean — a place you can visit, explore, and be moved by.

The goal is not merely to simulate an ocean, but to compress the feeling of standing at the edge of the unknown into something you can experience under your fingertips.

## Why This Exists

This project comes from a deep, lifelong fascination with the ocean and the underwater world.

As a child, the deep sea represented something both beautiful and unknowable — a vast, living mystery. That feeling has never left.

The desire is to build something that can give other people a similar lasting impression:

- The same awe Google Earth gave as a kid.
- The wonder of Minecraft's endless worlds.
- The immersive beauty of Subnautica, Finding Nemo, and Happy Feet.
- The quiet, life-shifting inspiration of films like *The Secret Life of Walter Mitty*.

Some experiences don't leave you. They stay for decades and quietly change how you see the world. SeaEngine aims for that kind of impact.

## The Vision

Imagine being able to:

- Dive to the seabed of the Mariana Trench in one moment.
- Surface at the Grand Reef the next.
- Witness artistic, majestic scenes — birds flying over a twilight sea, light shafts cutting through the water column, hydrothermal vents glowing in the darkness.

The core technological and artistic challenge is the **seamless "infinite" ocean simulation and exploration**.

The ocean should feel truly vast, alive, and continuous — not chopped into levels or loading zones. You should be able to travel great distances and feel the world change around you in meaningful, beautiful ways.

## Atmosphere as a Primary Pillar

One of the most powerful (and often underrated) aspects of *SpaceEngine* is its music and ambient sound design.

The right floating ambient during a long journey, or a melancholic melody when visiting a strange new place, can elevate the entire experience.

SeaEngine wants to do the same for the ocean:

- Proper music, ambient layers, and SFX that play **in context** of what is happening in the simulated world.
- The feeling of descending through the mesopelagic toward hydrothermal vents should be accompanied by sound that deepens the emotion.
- Audio should not be background decoration — it should be part of the world itself.

When atmosphere, visuals, and sound align, the experience can transcend tenfold.

## The Sublime and Grandness

Beyond realism, there is a desire for the **artistic and majestic**.

The project draws from experiences of "the sublime" — that grand, almost inexplicable feeling certain works evoke:

- The megastructure world of *Metal Garden*.
- The quiet emotional weight of *Ghost of Tsushima*.
- The intimate wonder of *Stray*.

SeaEngine should sometimes feel larger than life, even while remaining grounded in Earth's oceans. There is room for beauty that feels slightly heightened, poetic, and emotionally resonant.

## Grounded Mystery

The ocean is famously "only explored ~1%". That unknown carries weight.

SeaEngine wants to explore **subtle wonder and grounded mystery**, not cosmic horror or overt spectacle.

Later development will incorporate elements of survival and discovery — inspired by old classics like *Stranded 2* and its devlog dreams of being stranded on an island, only to slowly uncover strange, unexplained objects.

The tone is curiosity and quiet awe:
- "You've been here a while... and now you've found something odd on the beach. What could it be?"

Mystery should feel personal and wondrous, not threatening.

## Current Focus

Right now, the priority is the **sandbox exploration experience**:

- Building the seamless ocean world.
- Making movement and presence feel good.
- Establishing strong visual and atmospheric fidelity.
- Creating the technological foundation for "infinite" ocean simulation.

### Editor Integration

SeaEngine is a single executable (CMake target `seaengine`). The editor lives as a module under `seaengine/editor/`.

- **Explore Mode** (default): Immersive, SpaceEngine-like experience with authoring UI and editor-related systems disabled.
- Editor mode can be activated via the `--editor` command-line flag or an in-app menu.
- Editor-related systems are disabled when editor mode is switched off.
- Editor code is always compiled in (not Debug-only) but is not active by default.

This unified model keeps the default "Explore Mode" experience clean and focused on wonder while still providing full authoring capability when needed.

This sandbox (with integrated editor) is the base upon which everything else will be built.

## Long-Term Direction

SeaEngine is intended to become the foundation for a more directed experience:

- A game in which you play as a **blue whale**.
- Strong, captivating storytelling.
- Deeply immersive world and visuals.
- Blending the freedom of sandbox exploration with the emotional weight of a personal journey.

The blue whale protagonist is not just a vehicle — it is a way to experience the ocean from a creature's perspective: scale, movement, senses, and presence.

## Guiding Feelings

When someone spends time with SeaEngine (in sandbox form or later as a game), we hope they feel:

- Small in a beautiful way.
- Curious.
- Moved.
- Inspired.
- Like they have visited somewhere real and important, even if only for a little while.

The highest ambition is that the experience stays with people — quietly shaping how they see the ocean, exploration, and perhaps even their own lives.

---

## Key Inspirations

| Source                        | What Resonates                                      |
|-------------------------------|-----------------------------------------------------|
| SpaceEngine                   | Scale, exploration, ambient music, sense of wonder  |
| Google Earth                  | Childhood awe of seeing the real world digitally    |
| Subnautica                    | Underwater immersion and beauty                     |
| The Secret Life of Walter Mitty | Life-changing, inspiring quality of exploration   |
| Metal Garden                  | The sublime / grandness                             |
| Ghost of Tsushima             | Immersive world and emotional weight                |
| Stray                         | Intimate, atmospheric creature perspective          |
| Stranded 2 (and its devlogs)  | Grounded survival + subtle mystery and discovery    |
| Finding Nemo / Happy Feet     | Artistic beauty in oceanic settings                 |

---

## Closing Thought

SeaEngine is not just a technical project about simulating water.

It is an attempt to build a place that can inspire the same quiet awe and curiosity that the real ocean has inspired for centuries — and to give people a way to carry a piece of that feeling with them.

The ocean is still mostly unknown.

In this digital space, we get to explore it anyway.