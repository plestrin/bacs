#ifndef IRINFLUENCEMASK_H
#define IRINFLUENCEMASK_H

#include <stdint.h>

#include "graph.h"

uint64_t irInfluenceMask_operation_and(struct node* node, uint64_t mask);
#define  irInfluenceMask_operation_cmov(node, mask, dir) (mask)
#define  irInfluenceMask_operation_movzx(node, mask, dir) (mask)
#define  irInfluenceMask_operation_not(node, mask, dir) (mask)
uint64_t irInfluenceMask_operation_or(struct node* node, uint64_t mask);
#define irInfluenceMask_operation_part1_8(node, mask, dir) (mask)
#define irInfluenceMask_operation_part1_16(node, mask, dir) (mask)
uint64_t irInfluenceMask_operation_part2_8(struct node* node, uint64_t mask, uint32_t dir);
uint64_t irInfluenceMask_operation_rol(struct node* node, uint64_t mask, uint32_t dir);
#define  irInfluenceMask_operation_ror(node, mask, dir) irInfluenceMask_operation_rol(node, mask, edgeDirection_invert(dir))
uint64_t irInfluenceMask_operation_shl(struct node* node, uint64_t mask, uint32_t dir);
#define  irInfluenceMask_operation_shr(node, mask, dir) irInfluenceMask_operation_shl(node, mask, edgeDirection_invert(dir))
#define  irInfluenceMask_operation_xor(node, mask, dir) (mask)


#endif
