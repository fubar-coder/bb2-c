# Spec Delta

## Purpose

Gives programs that depend on the original Blitz Basic 2's overflowing Long and Quick products a way to get them back, at the cost of the operator, while `*` keeps the corrected products.

## ADDED Requirements

### Requirement: The Long function gives the original's product
The Long function SHALL return, for every pair of Long arguments, the value the original lmullib's Long multiplication gave for them.

#### Scenario: Overflowing cross product
- **WHEN** a program evaluates `LMulBB2(196608,30000)` and `LMulBB2(-196608,30000)`
- **THEN** it gets 1603272705 and -1603272705

#### Scenario: A generator
- **WHEN** a program computes `seed=LMulBB2(seed,69069)+1` twice from `seed=4711`
- **THEN** it gets 325384060, then 535264088, as with the original's `seed*69069+1`

#### Scenario: Products that fit
- **WHEN** a program evaluates `LMulBB2(a,b)` for a product that fits in a Long
- **THEN** it gets `a*b`

### Requirement: The Quick function gives the original's product
The Quick function SHALL return, for every pair of Quick arguments, the raw value the original lmullib's Quick multiplication gave for them.

#### Scenario: Overflowing integer parts
- **WHEN** a program evaluates `e.q=QMulBB2(300,300)`
- **THEN** the raw value of `e` is `$5F900001`

### Requirement: All pairs agree with the original
The two functions SHALL give the original's results for the 3000 mixed pairs of the analysis of #199 and the edge values 0, -1, $7FFFFFFF, $80000000 and the factors 2^k.

#### Scenario: Comparison with the original library
- **WHEN** the library test runs the functions over the vectors of lmullib's library test
- **THEN** every block equals the block the original lmullib gave for the same vectors

### Requirement: As fast as the operator
A call of either function SHALL take no more time than the original's operator for the same factors in a simple assignment.

#### Scenario: Benchmark
- **WHEN** the benchmark measures `r=LMulBB2(a,b)` and the original's `r=a*b`
- **THEN** the function's cycles per call are at most the original operator's

### Requirement: Nothing else changes
The operator `*`, lmullib and every executable that does not call the functions SHALL stay as before.

#### Scenario: A program without the functions
- **WHEN** a source that does not use the functions is compiled
- **THEN** the executable equals the previous commit's
