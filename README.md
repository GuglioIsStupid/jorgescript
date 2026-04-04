# Jorgescript

## What is this?

A **JOKE** LANGUAGE MADE BASED OFF MY FRIENDS DISCORD MESSAGE

<img src="./.github/theorigin.png">

The syntax has changed QUITE A LOT since the original version.

## But why?

> "I pride myself in making the worst things imaginable."

\- GuglioIsStupid June 21st 2023

## Documentation

### INFO

There is currently no extensive documentation currently as I am still trying to work out the syntax. Nothing is really meant to match, it's meant to be stupid the WHOLE way.

But here you can find some simple documentation for simple things.

### Syntax

The syntax is quite a mess to explain, but here is a written example of the syntax with explanations on what does what.

```
LOADCLIB "SDL3" AS SDL;
CLIBRETURNTYPE SDL::SDL_CreateWindow AS "SDL_Window";
CLIBRETURNTYPE SDL::SDL_HasEvent AS BOOLEAN;
ALWAYS SET SDL_EVENT_QUIT TO 0x100;

CALLCLIB SDL::SDL_Init(0x00000020);

SET SDLWINDOW TO SDL::SDL_CreateWindow(
    "JorgeScript SDL3 Window",
    800, 600,
    0
);

IF SDLWINDOW::IS(NOTHING) THEN {
    PRINT "Failed to create window!";
}

SET RUNNING TO TRUE!;

WHILE (RUNNING) {
    CALLCLIB SDL::SDL_PumpEvents();

    SET SHOULDQUIT TO SDL::SDL_HasEvent(SDL_EVENT_QUIT);
    IF SHOULDQUIT::IS(TRUE!) THEN {
        SET RUNNING TO Untrue...;
    }

    CALLCLIB SDL::SDL_Delay(16);
}

CALLCLIB SDL::SDL_DestroyWindow(SDLWINDOW);
CALLCLIB SDL::SDL_Quit();

```

`LOADCLIB` loads a native library into the code, works for both linux and windows, automatically fills the extension based on the os thats being used.

`CLIBRETURNTYPE` sets the return type of a specific function, if given a string, it is treated as a "POINTER".

`CALLCLIB` calls a function from the native library.

`ALWAYS SET X TO` is this languages equivelant of a constant. These values can not be changed.

`SET X TO` is how you set a modifiable variable.

`TRUE!` / `Untrue...` / `POTENTIONABLY` is this language's booleans, TRUE! represents... well true, Untrue represents false, and poitentionably represents either true/false randomly each time its compared.

`NOTHING` is how the language represents null variables.

`X::IS(TRUE!) THEN` lets you compare X to a variable, basically == in other languages. Normal operators like >=, <=, <, > can be used.

### Arrays

Arrays work similary to lua arrays, however, they are -1 indexed.

```
SET MYARRAY TO [1, 2, 3, 4, 5];
PRINT MYARRAY[0]; # prints 2
SET MYARRAY["NAME"] TO "JORGE";
PRINT MYARRAY["NAME"]; # prints JORGE
```

### STD Lib

Jorgescript has a STD Library implemented for use.

The lib allows you to access math, bit functions, and even io dialogs.

To use any of the functions, simple call `SUMMON "STD.MODULE"`

The following STD modules are available:

#### STD.MATH

- MATH::ABS
- MATH::MIN
- MATH::MAX
- MATH::CLAMP
- MATH::IS_INTEGER_HELPER
- MATH::NORMALIZE_ANGLE_HELPER
- MATH::SIN
- MATH::COS
- MATH::TAN
- MATH::ASIN
- MATH::ACOS
- MATH::ATAN
- MATH::ATAN2
- MATH::SQRT
- MATH::POW
- MATH::EXP
- MATH::LOG
- MATH::LOG10
- MATH::FLOOR
- MATH::CEIL
- MATH::ROUND
- MATH::SIGN

#### STD.STRING

- STRING::LEN, STRING::LENGTH
- STRING::SUBSTR, STRING::SUBSTRING
- STRING::FIND, STRING::INDEXOF
- STRING::REPLACE
- STRING::TRIM
- STRING::UPPER
- STRING::LOWER
- STRING::STARTSWITH
- STRING::ENDSWITH
- STRING::CONTAINS
- STRING::REPEAT
- STRING::JOIN
- STRING::SPLIT

#### STD.BIT

- BIT::AND
- BIT::BOR
- BIT::XOR
- BIT::BNOT
- BIT::SHL
- BIT::SHR

#### STD.IO

##### not currently in source, still in the works.

- IO::GET_CURRENT_PATH
- IO::INPUT, IO::PROMPT, IO::READ_LINE
- IO::READ_FILE
- IO::WRITE_FILE
- IO::APPEND_FILE
- IO::EXISTS
- IO::GET_CWD
- IO::FILE_SIZE

#### STD.TESTS

- TESTS::ASSERT_EQ, TESTS::ASSERT_EQUAL
