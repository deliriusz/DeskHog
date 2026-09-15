# Tamagotchi canonical-art prompt record

## Frozen identity bible

The canonical pet is an original, friendly **desk-hog** virtual pet: recognizably
hedgehog-like but not a recreation of a commercial virtual-pet character, Sonic, or
another mascot. Both stages use a full-body front three-quarter view facing
screen-right. Their shared silhouette is a compact pear-shaped torso, distinct
rounded face patch, short blunt snout, one small rounded visible ear, simple short
limbs, and a rear quill mass made of a few large, readable lobes. The face always
has two dark simple eyes with at most one tiny highlight each, a small dark nose at
the snout tip, and a one- or two-pixel-equivalent neutral-friendly mouth.

Two identity marks are mandatory in every later pose and life stage: a small cream
forehead notch above the visible eye and a simple oval cream belly patch. Quills are
chunky layered shapes with sparse, deliberate texture--never realistic individual
needles or noisy dithering. A consistent dark pixel-equivalent contour surrounds the
character and is thickened only where overlapping limbs require separation. Lighting
is a fixed upper-left flat color step, with no gradients, glow, rim light, or cast
shadow. The common palette has eight through twelve visible colors: one dark outline,
two or three quill browns, two warm face/belly tones, one highlight, one blush/accent,
and no more than two neutral support colors. The child and adult retain the same
palette roles. The neutral masters look alert, gentle, and slightly mischievous; both
feet rest on one shared ground line with no floor, ellipse, dust, or shadow.

The child is rounder, has the youngest head-to-body ratio, short limbs, a short snout,
large apparent eye area, and three or four blunt quill lobes. Phase 5 must be able to
place it at roughly 20--23 by 22--25 occupied pixels on a 32x32 canvas, visually
centered at x=16 with the lowest foot at y=28 and at least two transparent pixels at
the other extrema.

The adult is visibly the same individual: it keeps the exact face-patch geometry,
forehead notch, ear, eye spacing, snout direction, belly patch, palette, outline,
viewpoint, lighting, visual center, and ground line. It becomes adult only through a
two- or three-final-pixel-taller torso, slightly longer snout and limbs, a modestly
smaller apparent head-to-body ratio, and four or five developed chunky quill lobes.
It must not be a uniform scale-up, a muscular form, another species, or an accessory
variant. Phase 5 must be able to place it at roughly 23--26 by 25--28 occupied pixels
on the same 32x32 x=16/y=28 anchor.

## Shared negative constraints

Do not trace, copy, recolor, or reproduce the silhouette, anatomy, ground mark, or
specific face of the supplied walking frames; they are only references for hard pixel
edges, limited palette, outline clarity, and small-scale readability. Do not make any
existing Tamagotchi character, Sonic, or another branded mascot. Do not add clothing,
accessories, handheld identity props, detailed fingers or toes, realistic spines,
text, logos, watermarks, a second character, scenery, floor, cast shadow, glow,
gradient, soft painterly edge, antialiased halo, opaque background, or checkerboard.
Do not mirror the master, change its viewpoint, make one stage bipedal and the other
quadrupedal, alter face-mask geometry/palette roles, or use high-frequency texture.

## CHILD-CANONICAL-v1 -- exact prompt

~~~text
Use case: stylized-concept
Asset type: high-resolution pixel-art game character master for a 240x135 embedded TFT, to be inspected with nearest-neighbor reduction to 32x32 later; this is not a runtime sprite.
Primary request: create one original tiny child desk-hog virtual pet that is immediately readable after nearest-neighbor reduction to 32x32. The supplied walking sprite is a style reference only for pixel density, limited palette, hard outline, and small-scale readability. It is not an edit target: do not trace it, copy its silhouette, reproduce its anatomy, or reproduce its ground shadow.
Input images: Normal-Walking_01.png is a style-only reference, not an edit target.
Scene/backdrop: genuinely transparent background only, with economical transparent padding.
Subject: exactly one full-body original friendly desk-hog: a hedgehog-like virtual pet with a compact pear-shaped torso, distinct rounded warm face patch, short blunt snout facing screen-right, one small rounded visible ear, short simple limbs, and a rear quill mass of three or four large chunky readable lobes. Include two dark simple eyes with at most one tiny highlight each, a tiny dark nose at the snout tip, a one- or two-pixel-equivalent neutral-friendly mouth, a small cream forehead notch above the visible eye, and a simple oval cream belly patch. Both feet rest on the same implied ground line, but draw no ground or shadow.
Style/medium: crisp hand-authored-looking limited-palette pixel art, hard pixel edges, a consistent strong dark pixel-equivalent outline, sparse deliberate quill texture, and large contiguous face and belly shapes. Use eight through twelve high-contrast flat colors: dark outline, two or three warm quill browns, two warm face/belly tones, one cream highlight, one subtle blush/accent, and at most two neutral support colors. No gradients, glow, rim light, dithering noise, or realistic individual spines.
Composition/framing: one centered character in a full-body front three-quarter view facing screen-right. Keep it compact with the child’s round head and torso, largest apparent eye area, smallest snout, shortest limbs, and enough empty padding for later food, toy, bubble, and medical cues. Aim for a 32x32 normalized occupied area of about 20--23 pixels wide by 22--25 pixels high, visual center x=16, and lowest foot y=28, while leaving at least two transparent pixels around other extrema.
Lighting/mood: fixed upper-left lighting rendered only as flat color steps; alert, gentle, slightly mischievous, neutral-friendly expression.
Constraints: genuinely transparent alpha background; one character only; no text, logo, signature, watermark, scenery, floor, ellipse, dust, cast shadow, halo, glow, soft edge, opaque rectangle, or extra prop. Preserve the cream forehead notch and oval cream belly patch as permanent identity marks.
Avoid: existing Tamagotchi characters, Sonic, recognizable branded mascot traits, copying the supplied sprite, photorealism, painterly rendering, antialiasing, excessive detail, clothes, accessories, bipedal-only anatomy, detailed digits, and realistic needles/spines.
~~~

### Child revisions

#### CHILD-CANONICAL-v2 -- targeted correction: coarse binary-alpha rendering

~~~text
Use case: identity-preserve
Asset type: high-resolution pixel-art game character master for a 240x135 embedded TFT, to be inspected with nearest-neighbor reduction to 32x32 later; this is not a runtime sprite.
Primary request: retain the exact original child desk-hog identity in the supplied candidate, but correct only its rendering: redraw it as deliberately coarse, hand-authored pixel art on a visible logical pixel grid, using no more than twelve total visible flat colors and a genuinely transparent binary-alpha background. Every nontransparent pixel must be fully opaque; use no partial alpha, smoothing, blended edge colors, gradients, or antialiased halo. The final high-resolution master may enlarge this coarse grid with nearest-neighbor blocks, but it must remain crisp when reduced with nearest-neighbor to 32x32.
Input images: child-candidate-01.png is the edit/reference target. Preserve its original desk-hog identity rather than redesigning it.
Scene/backdrop: genuinely transparent background only, with economical transparent padding.
Subject: exactly one full-body original friendly child desk-hog: a hedgehog-like virtual pet with a compact pear-shaped torso, distinct rounded warm face patch, short blunt snout facing screen-right, one small rounded visible ear, short simple limbs, and a rear quill mass of three or four large chunky readable lobes. Preserve two dark simple eyes with at most one tiny highlight each, a tiny dark nose at the snout tip, a one- or two-pixel-equivalent neutral-friendly mouth, a small cream forehead notch above the visible eye, and a simple oval cream belly patch. Both feet rest on the same implied ground line, but draw no ground or shadow.
Style/medium: crisp hand-authored-looking limited-palette pixel art with a clearly intentional coarse logical pixel grid, hard pixel edges, a consistent strong dark pixel-equivalent outline, sparse deliberate quill texture, and large contiguous face and belly shapes. Use exactly ten or fewer high-contrast flat colors from these roles only: dark outline, two or three warm quill browns, two warm face/belly tones, one cream highlight, one subtle blush/accent, and at most two neutral support colors. Do not use gradients, glow, rim light, dithering noise, blended palette ramps, or realistic individual spines.
Composition/framing: one centered character in a full-body front three-quarter view facing screen-right. Keep it compact with the child’s round head and torso, largest apparent eye area, smallest snout, shortest limbs, and enough empty padding for later food, toy, bubble, and medical cues. Aim for a 32x32 normalized occupied area of about 20--23 pixels wide by 22--25 pixels high, visual center x=16, and lowest foot y=28, while leaving at least two transparent pixels around other extrema.
Lighting/mood: fixed upper-left lighting rendered only as single flat color steps; alert, gentle, slightly mischievous, neutral-friendly expression.
Constraints: genuinely transparent binary alpha background; one character only; no text, logo, signature, watermark, scenery, floor, ellipse, dust, cast shadow, halo, glow, soft edge, opaque rectangle, or extra prop. Preserve the cream forehead notch and oval cream belly patch as permanent identity marks.
Avoid: existing Tamagotchi characters, Sonic, recognizable branded mascot traits, copying the supplied walking sprite, photorealism, painterly rendering, any semitransparent pixel, antialiasing, excessive detail, clothes, accessories, bipedal-only anatomy, detailed digits, and realistic needles/spines.
~~~

#### CHILD-CANONICAL-v3 -- targeted correction: literal 10-swatch raster palette

~~~text
Use case: identity-preserve
Asset type: high-resolution pixel-art game character master for a 240x135 embedded TFT, to be inspected with nearest-neighbor reduction to 32x32 later; this is not a runtime sprite.
Primary request: retain the exact original child desk-hog identity in the supplied candidate, but correct only its technical raster palette. Render the image as literal hand-authored pixel art whose every nontransparent source pixel uses one of these exact ten RGB swatches only: outline #231612; deep-quill #593528; mid-quill #8C5033; light-quill #BE7544; face #E3A568; belly #FFD994; cream #FFF0BF; blush #F28C78; eye-white #FFFDF4; neutral #8A7568. Every nontransparent pixel must have alpha 255 and every transparent pixel alpha 0; do not create semitransparent pixels, blended colors, gradients, smoothing, or antialiased edges. The final high-resolution master may enlarge the logical pixels with nearest-neighbor blocks, but no additional colors or alpha values may appear.
Input images: child-candidate-02.png is the edit/reference target. Preserve its original desk-hog identity rather than redesigning it.
Scene/backdrop: genuinely transparent background only, with economical transparent padding.
Subject: exactly one full-body original friendly child desk-hog: a hedgehog-like virtual pet with a compact pear-shaped torso, distinct rounded warm face patch, short blunt snout facing screen-right, one small rounded visible ear, short simple limbs, and a rear quill mass of three or four large chunky readable lobes. Preserve two dark simple eyes with at most one tiny #FFFDF4 highlight each, a tiny #231612 nose at the snout tip, a one- or two-pixel-equivalent neutral-friendly mouth, a small #FFF0BF cream forehead notch above the visible eye, and a simple oval #FFF0BF cream belly patch. Both feet rest on the same implied ground line, but draw no ground or shadow.
Style/medium: crisp hand-authored-looking limited-palette pixel art with a clearly intentional coarse logical pixel grid, hard pixel edges, a consistent #231612 dark pixel-equivalent outline, sparse deliberate quill texture, and large contiguous face and belly shapes. Use the stated ten exact swatches only, with #593528/#8C5033/#BE7544 for quills and #E3A568/#FFD994/#FFF0BF for face and belly roles. Do not use gradients, glow, rim light, dithering noise, blended palette ramps, or realistic individual spines.
Composition/framing: one centered character in a full-body front three-quarter view facing screen-right. Keep it compact with the child’s round head and torso, largest apparent eye area, smallest snout, shortest limbs, and enough empty padding for later food, toy, bubble, and medical cues. Aim for a 32x32 normalized occupied area of about 20--23 pixels wide by 22--25 pixels high, visual center x=16, and lowest foot y=28, while leaving at least two transparent pixels around other extrema.
Lighting/mood: fixed upper-left lighting rendered only as stated single flat color steps; alert, gentle, slightly mischievous, neutral-friendly expression.
Constraints: genuinely transparent binary alpha background; one character only; no text, logo, signature, watermark, scenery, floor, ellipse, dust, cast shadow, halo, glow, soft edge, opaque rectangle, or extra prop. Preserve the #FFF0BF cream forehead notch and oval #FFF0BF cream belly patch as permanent identity marks.
Avoid: existing Tamagotchi characters, Sonic, recognizable branded mascot traits, copying the supplied walking sprite, photorealism, painterly rendering, any RGB color outside the stated swatches, any semitransparent pixel, antialiasing, excessive detail, clothes, accessories, bipedal-only anatomy, detailed digits, and realistic needles/spines.
~~~

#### CHILD-CANONICAL-v4 -- targeted correction: visible second eye at 32x32

~~~text
Use case: identity-preserve
Asset type: high-resolution pixel-art game character master for a 240x135 embedded TFT, to be inspected with nearest-neighbor reduction to 32x32 later; this is not a runtime sprite.
Primary request: retain the supplied child desk-hog exactly, but correct only the face readability defect: add a clearly separated second dark eye on the far side of the face patch. At 32x32, two dark eyes must remain visibly distinct rather than one eye merging with the ear or snout. Keep both eyes simple and large enough to read, with at most one tiny highlight per eye. Do not otherwise redesign the character.
Input images: child-candidate-03.png is the edit/reference target. Preserve its original desk-hog identity rather than redesigning it.
Scene/backdrop: genuinely transparent background only, with economical transparent padding.
Subject: exactly one full-body original friendly child desk-hog: a hedgehog-like virtual pet with a compact pear-shaped torso, distinct rounded warm face patch, short blunt snout facing screen-right, one small rounded visible ear, short simple limbs, and a rear quill mass of three or four large chunky readable lobes. Keep a tiny dark nose at the snout tip, a one- or two-pixel-equivalent neutral-friendly mouth, a small cream forehead notch above the visible eye, and a simple oval cream belly patch. Both feet rest on the same implied ground line, but draw no ground or shadow.
Style/medium: crisp hand-authored-looking limited-palette pixel art with a clearly intentional coarse logical pixel grid, hard pixel edges, a consistent strong dark pixel-equivalent outline, sparse deliberate quill texture, and large contiguous face and belly shapes. Use eight through twelve high-contrast flat colors: dark outline, two or three warm quill browns, two warm face/belly tones, one cream highlight, one subtle blush/accent, and at most two neutral support colors. No gradients, glow, rim light, dithering noise, blended palette ramps, or realistic individual spines.
Composition/framing: one centered character in a full-body front three-quarter view facing screen-right. Keep it compact with the child’s round head and torso, largest apparent eye area, smallest snout, shortest limbs, and enough empty padding for later food, toy, bubble, and medical cues. Aim for a 32x32 normalized occupied area of about 20--23 pixels wide by 22--25 pixels high, visual center x=16, and lowest foot y=28, while leaving at least two transparent pixels around other extrema.
Lighting/mood: fixed upper-left lighting rendered only as flat color steps; alert, gentle, slightly mischievous, neutral-friendly expression.
Constraints: genuinely transparent alpha background; one character only; no text, logo, signature, watermark, scenery, floor, ellipse, dust, cast shadow, halo, glow, soft edge, opaque rectangle, or extra prop. Preserve the cream forehead notch and oval cream belly patch as permanent identity marks.
Avoid: existing Tamagotchi characters, Sonic, recognizable branded mascot traits, copying the supplied walking sprite, photorealism, painterly rendering, antialiasing, excessive detail, clothes, accessories, bipedal-only anatomy, detailed digits, and realistic needles/spines.
~~~

## ADULT-CANONICAL-v1 -- exact edit prompt

~~~text
Use case: identity-preserve
Asset type: high-resolution pixel-art game character master for a 240x135 embedded TFT, to be inspected with nearest-neighbor reduction to 32x32 later; this is not a runtime sprite.
Primary request: create the single adult life-stage variant of this exact original desk-hog. Use the supplied accepted child master as the edit/reference target. Change only the life-stage anatomy: give the pet a modestly taller torso, slightly longer blunt snout and limbs, a slightly smaller apparent head-to-body ratio, and four or five more developed chunky quill lobes. The adulthood change must be visible at 32x32 but must not be a uniform scale-up, muscular form, different species, accessory variant, or different individual.
Input images: tamagotchi_child_reference.png is the edit/reference target and sole identity source.
Scene/backdrop: genuinely transparent background only, with economical transparent padding.
Subject: exactly one full-body original friendly desk-hog. Preserve exactly the child master’s rounded warm face-patch geometry, small cream forehead notch above the visible eye, small rounded visible ear, dark eye spacing and one-tiny-highlight maximum, short blunt snout direction toward screen-right, small dark nose, neutral-friendly one- or two-pixel mouth, simple oval cream belly patch, palette roles, dark pixel-equivalent outline language, front three-quarter viewpoint, fixed upper-left flat-step lighting, visual center, and shared foot ground line. Keep the character compact and cute; both feet rest on that same implied ground line, without drawing any ground or shadow.
Style/medium: crisp hand-authored-looking limited-palette pixel art, hard pixel edges, a consistent strong dark pixel-equivalent outline, sparse deliberate quill texture, and large contiguous face and belly shapes. Keep exactly the child master’s palette roles: dark outline, two or three warm quill browns, two warm face/belly tones, one cream highlight, one subtle blush/accent, and at most two neutral support colors. No gradients, glow, rim light, dithering noise, or realistic individual spines.
Composition/framing: one centered full-body front three-quarter character facing screen-right. Leave space for later generic food, toy, bubble, and medical cues. Aim for a 32x32 normalized occupied area of about 23--26 pixels wide by 25--28 pixels high, visual center x=16, and lowest foot y=28, with at least two transparent pixels around all other extrema.
Lighting/mood: the exact same fixed upper-left flat color-step lighting and alert, gentle, slightly mischievous neutral-friendly expression as the child.
Constraints: genuinely transparent alpha background; one character only; no text, logo, signature, watermark, scenery, floor, ellipse, dust, cast shadow, halo, glow, soft edge, opaque rectangle, or extra prop. Preserve the face patch, forehead notch, ear, eye spacing, snout direction, belly patch, palette, outline, viewpoint, light direction, center, and ground line.
Avoid: existing Tamagotchi characters, Sonic, recognizable branded mascot traits, a different character, copying the supplied walking sprite, photorealism, painterly rendering, antialiasing, excessive detail, clothes, accessories, bipedal-only anatomy, detailed digits, realistic needles/spines, a uniform child scale-up, or an opaque/checkerboard background.
~~~

### Adult revisions

No adult revision was required. `ADULT-CANONICAL-v1` was accepted after direct
source-size, alpha, compositing, and child/adult nearest-neighbor preview review.

## Accepted palette

| Role | Hex |
| --- | --- |
| outline | `#231612` |
| deep quill | `#593528` |
| neutral support | `#8A7568` |
| mid quill | `#8C5033` |
| light quill | `#BE7544` |
| warm face | `#E3A568` |
| blush accent | `#F28C78` |
| warm belly | `#FFD994` |
| cream identity mark | `#FFF0BF` |
| eye highlight | `#FFFDF4` |

## Accepted-master source-cleanup exception

The built-in generator’s unmodified PNGs had blended RGB values and broad
intermediate-alpha regions even after the palette/binary-alpha prompts. On 2026-09-15
the project owner explicitly approved source-only cleanup of the selected masters. No
runtime asset, file in `raw-png/`, or generated LVGL descriptor was created or edited.

The accepted child derives from `CHILD-CANONICAL-v4`; the adult derives from the
accepted child through `ADULT-CANONICAL-v1`. For each source master, cleanup first
retained only alpha values at least 128 as opaque, mapped every retained RGB value to
the exact accepted palette by nearest RGB-distance, and discarded all other pixels.
The result was reduced into the approved 32x32 logical grid and then enlarged 32x with
nearest-neighbor to a 1024x1024 RGBA reference master. Child content is 23x25 pixels
at x=5..27 and y=4..28; adult content is 25x27 pixels at x=4..28 and y=2..28. This
fixes the final visual center and common y=28 ground line without creating a runtime
sprite or changing later Phase 5 ownership of runtime-frame normalization.

Rejected child source candidates were retained only under
`/tmp/deskhog-tamagotchi-phase3/`: v1 had 24,894 visible RGB values and 256 alpha
values; v2 had 22,771 visible RGB values and 256 alpha values; v3 had 23,635 visible
RGB values and 256 alpha values. They were rejected before promotion.

## Generation provenance

Generation used the OpenAI built-in image-generation tool. The exposed C2PA metadata
identified the software agent as `gpt-image` version `2.0`; the exposed generation
artifact filenames were `exec-d707c97f-444d-4956-b26b-2cf5128d5efa.png` for the
accepted child revision and `exec-5bb0a7a5-7421-4517-badf-2a9e3b61e74d.png` for the
adult edit. The manifest records only repository-relative accepted paths and input
hashes; rejected candidates remain outside the repository.

## Phase 4 handoff

Use the matching accepted master as the only identity source for every later generated
strip. Preserve face-patch geometry, cream forehead notch, ear, eye spacing, snout
direction, oval cream belly patch, palette roles, hard outline, front three-quarter
screen-right viewpoint, upper-left lighting, x=16 visual center, and y=28 foot ground
line. Later strips may change only pose, a tiny expression, or the specifically
required generic prop; no frame may redesign the face, silhouette, life-stage
relationship, or palette. Keep limbs and body shifts small enough that a one-pixel
vertical bob and localized limb motion remain readable at 32x32.
