# Normalization Guide

## List of Operations

| Operation | Arity | Description |
| --------- | ----- | ----------- |
| ADC | 2 | Modular addition with carry. Incorrect: should take an extra argument for the carry. Do not delete as a subexpression. |
| ADD | > 1 | Modular addition. |
| AND | > 1 | Bitwise AND. |
| CMOV | 2 | Conditional move. Incorrect: should take an extra argument for FLAGS. Do not delete as a subexpression. |
| DIVQ | 2 | Division, returns the quotient. |
| DIVR | 2 | Division, returns the remainder. |
| IDIVQ | 2 | Signed division, returns the quotient. |
| IDIVR | 2 | Signed division, returns the remainder. |
| IMUL | > 1 | Signed modular multiplication. |
| MOVZX | 1 | Increase the size of variable and pad with zeroes.|
| MUL | > 1 | Modular multiplication. |
| NEG | 1 | Two's complement.|
| NOT | 1 | Bitwise NOT. |
| OR | > 1 | Bitwise OR. |
| PART1_8 | 1 | Takes the 8 least significant bits. |
| PART2_8 | 1 | Takes bit segment [8:15] starting from the least significant bits. |
| PART1_16 | 1 | Takes the 16 least significant bits. |
| ROL | 2 | Rotate to the left. |
| ROR | 2 | Rotate to the right. |
| SBB | 2 | Modular substitution with borrow. Incorrect: should take an extra argument for the borrow. Dot not delete as a subexpression. |
| SHL | 2 | Shift to the left. |
| SHLD | 3 | Shift to the left and shift in from the right bits from the third operand. |
| SHR | 2 | Shift to the right. |
| SHRD | 3 | Shift to the right and shift in from the left bits from the third operand. |
| SUB | 2 | Modular subtraction. |
| XOR | > 1 | Bitwise XOR. |

## List of the Normalization Mechanisms

* Constant Folding
* Constant Expression Detection
* Miscellaneous Rewrite Rules
* Common Subexpression Elimination
* Memory Access Simplification
* Constant Distribution
* Operation Size Expansion
* Memory Coalescing
* Affine Expression Simplification
* Constant Merging

## Constant Distribution

## Constant Expression Detection

If according to range value analysis an expression is constant, replace the expression by the constant.

## Miscellaneous Rewrite Rules

### ADD:
* Complex rules based on merging: use diff value, final flag and cst*

### AND:
* If there is a constant operand and if it does not modify the set of reachable values -> remove it from the operand list;

### MOVZX
* If the operand is PART1_8, PART1_16 and the size of its operand is equal to its size -> replace both instructions by AND;
* If the operand is PART2_8 and the size of its operand is equal to its size -> replace both instructions by SHR and AND.
*Do we really need this rule (it seems to me that it is redundant with what is done in irVaraiableSize)? Not really, but maybe it should ...*

### MUL
* If there are only two operands and one is a constant that is equal to a power of 2 -> replace by SHL.

### OR
* If several operands are equal -> keep only on of them ;
* If an operand is a OR with a single outgoing edge -> merge.
*This last rule should be moved to the MERGE generic pattern*

### PART1_8
* If the operand is MOVZX -> replace the current node by MOVZ operand ;
* If the operand is PART1_16 -> replace in the operand list PART1_16 by its operand.

### PART2_8
* If the operand is PART1_16 -> replace in the operand list PART1_16 by its operand.

### ROL
* If the disp operand is a constant -> replace by ROR (adjust the value of the disp operand).

### SHL
* If the direct operand is SHR or SHL and if the disp operand is a constant -> replace by a single shift with a masking operation if necessary.

### SHLD
* If both the first and the third operand are equal -> replace by ROL.

### SHR
* If the direct operand is SHR or SHL and if the disp operand is a constant -> replace by a single shift with a masking operation if necessary.

### SHRD
* If both the first and the third operand are equal -> replace by ROL.

### SUB
* If the sub operand is a constant -> replace by and ADD.

### XOR
* If two operands are equal -> remove both (if the operand count fall to zero, replace by a constant);
* If there are two operand and one of them is a constant equal to 0xff..ff -> replace by NOT.
*Complex rules based on merging: use diff value and final flag*

## Operation Size Expansion

1. Expand sizes. For every operation in that list: {ADC, ADD, AND, CMOV, NEG, NOT, OR, SBB, SHL, SUB, XOR} if the size is smaller than the ideal size (which is equal to 32 bits), we increase the size. We also increase the size of constant vertices if the sizes of their direct successors were increased.
2. Delete obsolete size modifiers. Size modifiers that are not required any more (that is to say the size of theirs direct predecessor is equal to the size of their direct successor(s)) are deleted. Special actions are performed while deleting MOVZX and PART2_8 operations.
3. Insert new size modifiers. We insert new size modifiers where it is necessary.

## Constant Merging

For now on, only two operations are affected  by constant merging: ADD and AND. If two operations (one being a direct successor of the other) both have a constant operand, we merge them. Warning, this normalization mechanism can cause an infinite loop with common subexpression elimination. Constant folding have to be executed before.


## Topological Order

| Rules | Order | Remarks |
| ----- | ----- | ------- |
| Constant Folding | DST -> SRC | Safe node iteration (only deletes operands). |
| Dead Code Removal | DST -> SRC | Mandatory to delete everything in a single pass. |
| Cst Exp Detection | DST -> SRC | Influence mask computation and safe node iteration (only deletes operands). |
| Cst Distribution | No |Does not require any ordering, does not preserve ordering. |
| Cst Merging | No | Does not require any ordering, does not preserve ordering. |
| Misc Rewrite Rules | DST -> SRC | Safe node iteration. |
| Remove Subexpression | SRC -> DST | Benefit from a cascading effect. The order is not preserved. |
| Operation Size Exp | DST -> SRC | Safe node iteration. |
