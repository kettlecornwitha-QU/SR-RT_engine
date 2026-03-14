# Video Formula Guide

This document explains how formulas work in the `Video Renderer`.

The Video Renderer lets you define:
- reusable named expressions
- camera location formulas
- camera velocity formulas
- camera orientation formulas
- frame ranges where different formulas apply

## Built-in Variables

These names are always available in formulas:

- `frame`
  - current frame number
- `fps`
  - frames per second selected in the GUI
- `time`
  - defined as `frame / fps`

Example:

```text
time * 2
```

## Definitions

The `Definitions` section lets you create named expressions of the form:

```text
name = expression
```

Examples:

```text
start = 76
end = -150
distance = abs(end - start)
halfway = arcosh(distance / 2 + 1)
```

Definitions are evaluated in order from top to bottom.

That means later definitions can depend on earlier ones:

```text
a = 2
b = a + 3
```

but not the other way around.

## Supported Operators

The formula evaluator supports:

- addition: `+`
- subtraction: `-`
- multiplication: `*`
- division: `/`
- floor division: `//`
- modulo: `%`
- exponentiation: `^` or `**`

Examples:

```text
2 + 3
10 / 4
frame % 60
(x + y)^2
```

Notes:
- in the GUI, `^` is accepted and translated internally to exponentiation
- implicit multiplication is not supported
- write `2*x`, not `2x`

## Supported Constants

These math constants are built in:

- `pi`
- `e`

Examples:

```text
2*pi
exp(1) == e
```

## Supported Functions

The evaluator currently supports:

- `sqrt(x)`
- `abs(x)`
- `int(x)`
- `floor(x)`
- `ceil(x)`
- `round(x)`
- `sin(x)`
- `cos(x)`
- `tan(x)`
- `asin(x)`
- `acos(x)`
- `atan(x)`
- `sinh(x)`
- `cosh(x)`
- `tanh(x)`
- `asinh(x)`
- `acosh(x)`
- `atanh(x)`
- `arsinh(x)`
- `arcosh(x)`
- `artanh(x)`
- `log(x)`
- `log10(x)`
- `exp(x)`
- `min(a, b, ...)`
- `max(a, b, ...)`

Important note:
- `arsinh`, `arcosh`, and `artanh` are accepted aliases for:
  - `asinh`
  - `acosh`
  - `atanh`

## Expression Rules

The evaluator is intentionally limited and safe:
- formulas are parsed as expressions only
- arbitrary Python code is not allowed
- unsupported names cause an error
- unsupported functions cause an error

Typical errors include:
- `Unknown symbol: name`
- `Unsupported function: name`
- `Syntax error: ...`

## Camera Sections

The Video Renderer has three timed formula sections:

1. `Camera Location`
   - `x`, `y`, `z`
2. `Camera Velocity (scene frame)`
   - `x`, `y`, `z`
3. `Camera Orientation (turns)`
   - `pitch`, `yaw`

Each row in these sections contains formulas for the listed components.

Examples:

```text
x = 0
y = 1.2
z = start - cosh(frame / fps) - 1
```

```text
x = 0
y = 0
z = tanh(frame / fps)
```

```text
pitch = 0
yaw = frame / fps / halfway
```

## Frame Ranges

Each camera section supports multiple timed rows.

The meaning of the ranges is:

- first row:
  - `frame 0` through and including the entered end frame
- middle rows:
  - `frame 1 + [previous end]` through and including the entered end frame
- last row:
  - `frame 1 + [previous end]` to the end of the render

So if you have two rows and the first row range is:

```text
119
```

then:
- row 1 applies to frames `0` through `119`
- row 2 applies to frames `120` through the end

## Total Number of Frames

The `Total number of frames` field is also a formula.

It can use:
- numbers
- built-in variables like `fps`
- your definitions

Examples:

```text
240
```

```text
int(2 * 60 * halfway)
```

The value must evaluate to a positive integer.

## Velocity Interpretation

Camera velocity formulas are entered as:

- `x`, `y`, `z` components in the **scene frame**

The renderer converts those components into:
- aberration speed
- aberration yaw
- aberration pitch

So the Video Renderer formula input for velocity is not:
- speed + yaw + pitch

It is:
- world/scene-frame vector components

Example:

```text
x = 0.5
y = 0
z = -0.5
```

This will be converted internally into the aberration representation used by the renderer.

## Orientation Units

Camera orientation uses **turns**, not radians or degrees.

That means:
- `1.0` turn = full rotation
- `0.5` turn = half rotation
- `0.25` turn = quarter rotation

Examples:

```text
yaw = 0.125
```

means one eighth of a full turn.

## Example: Hyperbolic Motion

This is a simplified example in the same style as your existing presets.

Definitions:

```text
start = 76
end = -150
distance = abs(end - start)
halfway = arcosh(distance / 2 + 1)
```

Total frames:

```text
int(2 * 60 * halfway)
```

Camera location row 1:

```text
x = 0
y = 1.2
z = start - cosh(frame / fps) - 1
```

Range end:

```text
int(60 * halfway)
```

Camera location row 2:

```text
x = 0
y = 1.2
z = cosh(frame / fps - 2 * halfway) - 1 + start - distance
```

Camera velocity row 1:

```text
x = 0
y = 0
z = tanh(frame / fps)
```

Range end:

```text
int(60 * halfway)
```

Camera velocity row 2:

```text
x = 0
y = 0
z = -tanh(frame / fps - 2 * halfway)
```

Camera orientation:

```text
pitch = 0
yaw = 0
```

## Example: Rotating Camera

To make the camera rotate while rendering:

```text
pitch = 0
yaw = frame / fps / halfway
```

If this exceeds `1.0`, that is fine. Turns wrap naturally as the renderer uses the value as an angle measure, not as a clamped percentage.

## Practical Tips

- define reusable names early in `Definitions`
- use `abs(...)` for distances
- use `int(...)` when a field expects an integer-like frame count
- use `^` if that feels more natural; the evaluator will convert it
- when debugging, simplify formulas to constants first

Good debugging pattern:

1. replace a formula with a constant like `0`
2. confirm the render starts
3. reintroduce one variable at a time

## Current Limits

- no implicit multiplication
- no conditionals like `if ... else ...`
- no vector literals
- no user-defined functions
- no direct speed/yaw/pitch input mode for video formulas

Those limits are intentional to keep the evaluator predictable and safe.
