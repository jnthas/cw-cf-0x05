#include "Clockface.h"

// Check if a 5x5 entity at (x,y) can move 1px in dir without overlapping WALL or CLOCK
bool Clockface::canMove(int x, int y, Direction dir) {
    int newX = x, newY = y;
    if (dir == Direction::RIGHT)      newX += 1;
    else if (dir == Direction::LEFT)  newX -= 1;
    else if (dir == Direction::DOWN)  newY += 1;
    else if (dir == Direction::UP)    newY -= 1;

    // Check all 25 pixels of the 5x5 sprite at the new position
    for (int py = newY; py < newY + 5; py++) {
        for (int px = newX; px < newX + 5; px++) {
            int gridR = pixelToGrid(py);
            int gridC = pixelToGrid(px);
            if (gridR < 0 || gridR >= MAP_SIZE || gridC < 0 || gridC >= MAP_SIZE) {
                return false;
            }
            MapBlock block = static_cast<MapBlock>(_MAP[gridR][gridC]);
            if (block == MapBlock::WALL || block == MapBlock::CLOCK) {
                return false;
            }
        }
    }
    return true;
}

// Returns true when the entity is exactly aligned to a grid junction on both axes
bool Clockface::isAtGridJunction(int x, int y) {
    return (x - 2) % 5 == 0 && (y - 2) % 5 == 0;
}

// Helper function to check if a cell is within bounds and movable
bool Clockface::isValid(int r, int c) {
    // Check bounds
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }
    // Check if the block type is movable
    MapBlock block = static_cast<MapBlock>(_MAP[r][c]);
    // Allow moving onto EMPTY, FOOD, GATE (via contains) OR SUPER_FOOD
    return contains(block, PACMAN_MOVING_BLOCKS) || block == MapBlock::SUPER_FOOD;
}

// Helper function to check if a cell contains a target (food or superfood)
bool Clockface::isTarget(int r, int c) {
    // Check bounds (although isValid should handle this)
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }
    MapBlock block = static_cast<MapBlock>(_MAP[r][c]);
    return block == MapBlock::FOOD || block == MapBlock::SUPER_FOOD;
}

bool Clockface::isNormalFoodTarget(int r, int c) {
    if (r < 0 || r >= MAP_SIZE || c < 0 || c >= MAP_SIZE) {
        return false;
    }
    return static_cast<MapBlock>(_MAP[r][c]) == MapBlock::FOOD;
}

bool Clockface::hasAdjacentNormalFood(int r, int c) {
    for (int i = 0; i < 4; ++i) {
        int nextR = r + DIR_OFFSETS[i].dRow;
        int nextC = c + DIR_OFFSETS[i].dCol;
        if (isNormalFoodTarget(nextR, nextC)) {
            return true;
        }
    }
    return false;
}




// Picks the direction that maximizes Manhattan distance from a threat.
// Prefers non-reversal directions (no U-turn).
// Falls back to allowing reversal, then to any valid direction.
Direction Clockface::fleeDirection(int entityR, int entityC, Direction currentDir,
                                    int threatR, int threatC, bool allowUTurn) {
    Direction bestDir = currentDir;
    int maxDist = -1;
    Direction opposite = oppositeDirection(currentDir);

    for (int i = 0; i < 4; ++i) {
        int nextR = entityR + DIR_OFFSETS[i].dRow;
        int nextC = entityC + DIR_OFFSETS[i].dCol;
        if (isValid(nextR, nextC)) {
            if (!allowUTurn && static_cast<Direction>(i) == opposite) {
                continue;
            }
            int d = abs(nextR - threatR) + abs(nextC - threatC);
            if (d > maxDist) {
                maxDist = d;
                bestDir = static_cast<Direction>(i);
            }
        }
    }

    if (maxDist == -1) {
        for (int i = 0; i < 4; ++i) {
            int nextR = entityR + DIR_OFFSETS[i].dRow;
            int nextC = entityC + DIR_OFFSETS[i].dCol;
            if (isValid(nextR, nextC)) {
                int d = abs(nextR - threatR) + abs(nextC - threatC);
                if (d > maxDist) {
                    maxDist = d;
                    bestDir = static_cast<Direction>(i);
                }
            }
        }
    }

    return bestDir;
}

// Multi-ghost flee vector: computes repulsion from all ghosts within 4 tiles,
// weighted by proximity (task 2.1)
Direction Clockface::fleeDirectionMulti(int entityR, int entityC, Direction currentDir, bool allowUTurn) {
    float sumR = 0, sumC = 0;

    for (int gi = 0; gi < _ghostCount; gi++) {
        int gr = pixelToGrid(ghosts[gi]->getY());
        int gc = pixelToGrid(ghosts[gi]->getX());
        int dist = abs(entityR - gr) + abs(entityC - gc);

        if (dist <= 4 && dist > 0) {
            // Repulsion vector = away from ghost
            float weight = 1.5f - ((float)dist / 4.0f);
            sumR += (entityR - gr) * weight;
            sumC += (entityC - gc) * weight;
        }
    }

    // If no ghosts were within range, return current direction
    if (sumR == 0 && sumC == 0) {
        return currentDir;
    }

    // Task 1.1: Zero-vector cancellation — ghosts symmetrically positioned (pincer)
    if (fabs(sumR) < 0.001f && fabs(sumC) < 0.001f) {
        int ghostR, ghostC;
        int idx = nearestGhost(entityR, entityC, ghostR, ghostC);
        if (idx >= 0) {
            return fleeDirection(entityR, entityC, currentDir, ghostR, ghostC, /*allowUTurn=*/true);
        }
        return currentDir;
    }

    // Pick the cardinal direction closest to the summed vector
    // that is not blocked and not a reversal (unless allowUTurn)
    Direction bestDir = currentDir;
    float bestDot = -9999;
    Direction opposite = oppositeDirection(currentDir);

    struct { int dRow, dCol; } dirVecs[4] = {{0, 1}, {0, -1}, {-1, 0}, {1, 0}};  // RIGHT, LEFT, UP, DOWN

    for (int i = 0; i < 4; i++) {
        if (!allowUTurn && static_cast<Direction>(i) == opposite) continue;

        int nextR = entityR + dirVecs[i].dRow;
        int nextC = entityC + dirVecs[i].dCol;
        if (!isValid(nextR, nextC)) continue;

        // Dot product of direction unit vector with summed repulsion vector
        float dot = dirVecs[i].dRow * sumR + dirVecs[i].dCol * sumC;
        if (dot > bestDot) {
            bestDot = dot;
            bestDir = static_cast<Direction>(i);
        }
    }

    // If no direction was valid (shouldn't happen often), allow reversal as fallback
    if (bestDot == -9999) {
        for (int i = 0; i < 4; i++) {
            int nextR = entityR + dirVecs[i].dRow;
            int nextC = entityC + dirVecs[i].dCol;
            if (isValid(nextR, nextC)) {
                bestDir = static_cast<Direction>(i);
                break;
            }
        }
    }

    return bestDir;
}

int Clockface::directionSafetyScore(int entityR, int entityC, Direction dir) {
    int nextR = entityR + DIR_OFFSETS[static_cast<int>(dir)].dRow;
    int nextC = entityC + DIR_OFFSETS[static_cast<int>(dir)].dCol;
    if (!isValid(nextR, nextC)) {
        return -1;
    }

    int minGhostDist = 999;
    for (int gi = 0; gi < _ghostCount; gi++) {
        int gr = pixelToGrid(ghosts[gi]->getY());
        int gc = pixelToGrid(ghosts[gi]->getX());
        int dist = abs(nextR - gr) + abs(nextC - gc);
        if (dist < minGhostDist) {
            minGhostDist = dist;
        }
    }
    return minGhostDist;
}

Direction Clockface::preferFoodWhileFleeing(int entityR, int entityC, Direction fleeDir, int safetyMargin) {
    int fleeScore = directionSafetyScore(entityR, entityC, fleeDir);
    Direction bestFoodDir = fleeDir;
    int bestFoodScore = -1;

    for (int i = 0; i < 4; ++i) {
        Direction tryDir = static_cast<Direction>(i);
        int nextR = entityR + DIR_OFFSETS[i].dRow;
        int nextC = entityC + DIR_OFFSETS[i].dCol;
        if (!isValid(nextR, nextC) || !canMove(pacman->getX(), pacman->getY(), tryDir)) {
            continue;
        }

        if (static_cast<MapBlock>(_MAP[nextR][nextC]) != MapBlock::FOOD) {
            continue;
        }

        int foodScore = directionSafetyScore(entityR, entityC, tryDir);
        if (foodScore < 0) {
            continue;
        }

        if (foodScore + safetyMargin >= fleeScore && foodScore > bestFoodScore) {
            bestFoodScore = foodScore;
            bestFoodDir = tryDir;
        }
    }

    return bestFoodDir;
}

// Task 2.1: Evaluates all four cardinal directions and scores each by the minimum
// Manhattan distance from the resulting cell to any ghost.
// Returns the direction with the highest safety score.
// When safety scores are tied, prefers directions whose adjacent cell has food.
Direction Clockface::safestDirection(int entityR, int entityC) {
    Direction bestDir = Direction::RIGHT; // fallback
    int bestScore = -1;
    bool bestHasFood = false;

    for (int i = 0; i < 4; ++i) {
        int nextR = entityR + DIR_OFFSETS[i].dRow;
        int nextC = entityC + DIR_OFFSETS[i].dCol;
        if (!isValid(nextR, nextC)) continue;

        // Score = minimum Manhattan distance from resulting cell to any ghost
        int minGhostDist = 999;
        for (int gi = 0; gi < _ghostCount; gi++) {
            int gr = pixelToGrid(ghosts[gi]->getY());
            int gc = pixelToGrid(ghosts[gi]->getX());
            int dist = abs(nextR - gr) + abs(nextC - gc);
            if (dist < minGhostDist) {
                minGhostDist = dist;
            }
        }

        // Check if the adjacent cell has food (tiebreaker)
        MapBlock blk = static_cast<MapBlock>(_MAP[nextR][nextC]);
        bool hasFood = (blk == MapBlock::FOOD || blk == MapBlock::SUPER_FOOD);

        // Prefer higher safety score; on tie, prefer food
        if (minGhostDist > bestScore ||
            (minGhostDist == bestScore && hasFood && !bestHasFood)) {
            bestScore = minGhostDist;
            bestDir = static_cast<Direction>(i);
            bestHasFood = hasFood;
        }
    }

    // If no valid direction was found (shouldn't happen on this map), fall back
    if (bestScore < 0) {
        for (int i = 0; i < 4; ++i) {
            int nextR = entityR + DIR_OFFSETS[i].dRow;
            int nextC = entityC + DIR_OFFSETS[i].dCol;
            if (isValid(nextR, nextC)) {
                return static_cast<Direction>(i);
            }
        }
    }

    return bestDir;
}

// Task 4.1: Computes the dot product of a ghost's movement direction with the
// vector from the ghost toward the entity (Pacman).
// Returns > 0 if the ghost is moving toward the entity, < 0 if moving away.
int Clockface::ghostDirectionDotProduct(Direction ghostDir, int dr, int dc) {
    int dirRow = DIR_OFFSETS[static_cast<int>(ghostDir)].dRow;
    int dirCol = DIR_OFFSETS[static_cast<int>(ghostDir)].dCol;
    return dirRow * dr + dirCol * dc;
}

void Clockface::ghostDirectionDecision(Ghost* g) {
    int ghostR = pixelToGrid(g->getY());
    int ghostC = pixelToGrid(g->getX());
    int pacmanR = pixelToGrid(pacman->getY());
    int pacmanC = pixelToGrid(pacman->getX());

    Direction nextMove = g->_direction;

    if (pacman->_state == Pacman::State::INVENCIBLE) {
        // Ghosts flee from Pacman when he's invincible
        // Reverse only when entering frightened mode. Allowing a U-turn at
        // every junction makes a ghost oscillate between two cells.
        bool enteringFrightenedMode = g->_state != Ghost::FRIGHTENED;
        Direction bestDir = fleeDirection(ghostR, ghostC, g->_direction, pacmanR, pacmanC,
                                         /*allowUTurn=*/enteringFrightenedMode);
        if (bestDir != g->_direction) {
            if (canMove(g->getX(), g->getY(), bestDir)) {
                g->turn(bestDir);
            }
        } else if (!canMove(g->getX(), g->getY(), bestDir)) {
            // Current direction is pixel-blocked — find any valid alternate
            for (int i = 0; i < 4; i++) {
                Direction tryDir = static_cast<Direction>(i);
                if (tryDir != g->_direction && canMove(g->getX(), g->getY(), tryDir)) {
                    g->turn(tryDir);
                    break;
                }
            }
        }
    } else {
        // Alternating Chase / Scatter modes (20-second cycle: 14s Chase, 6s Scatter)
        bool scatterMode = (millis() / 1000) % 20 >= 14;
        int targetR = pacmanR;
        int targetC = pacmanC;

        if (scatterMode) {
            // Per-personality scatter targets (task 1.5)
            if (g->_personality == Ghost::BLINKY) {
                targetR = 0;
                targetC = 11;  // top-right corner
            } else { // PINKY
                targetR = 11;
                targetC = 0;   // bottom-left corner
            }
        } else {
            // Chase mode — branch on personality (task 1.3)
            if (g->_personality == Ghost::BLINKY) {
                // Blinky targets Pacman's current cell directly
                targetR = pacmanR;
                targetC = pacmanC;
            } else { // PINKY
                // Pinky uses ambush target (task 1.4)
                pinkyAmbushTarget(pacmanR, pacmanC, pacman->_direction, targetR, targetC);
            }
        }

        bool pathFound = findPathTo(ghostR, ghostC, targetR, targetC, nextMove);
        if (pathFound) {
            // Anti-reversal: prevent U-turn during normal chase/scatter (task 2.2)
            Direction oppositeDir = oppositeDirection(g->_direction);
            if (nextMove == oppositeDir) {
                Direction bestAlt = nextMove;
                int bestDist = 999;
                for (int i = 0; i < 4; i++) {
                    Direction tryDir = static_cast<Direction>(i);
                    if (tryDir == oppositeDir) continue; // Skip reverse
                    int tryR = ghostR + DIR_OFFSETS[i].dRow;
                    int tryC = ghostC + DIR_OFFSETS[i].dCol;
                    if (!isValid(tryR, tryC)) continue;
                    int d = abs(tryR - targetR) + abs(tryC - targetC);
                    if (d < bestDist) {
                        bestDist = d;
                        bestAlt = tryDir;
                    }
                }
                if (bestDist < 999) {
                    nextMove = bestAlt; // Use best non-reverse direction
                }
                // If bestDist == 999 (dead-end), allow U-turn as last resort
            }

            if (canMove(g->getX(), g->getY(), nextMove)) {
                // BFS path found and pixel-validated — turn toward target
                if (nextMove != g->_direction) {
                    g->turn(nextMove);
                }
            } else {
                // BFS succeeded but direction is pixel-blocked (sprite overlaps wall)
                // Try all 4 directions via canMove, preferring non-reverse directions (task 2.1)
                Direction opposite = oppositeDirection(g->_direction);
                bool turned = false;
                for (int i = 0; i < 4 && !turned; i++) {
                    Direction tryDir = static_cast<Direction>(i);
                    if (tryDir == opposite) continue; // Skip reverse on first pass
                    if (canMove(g->getX(), g->getY(), tryDir)) {
                        if (tryDir != g->_direction) {
                            g->turn(tryDir);
                        }
                        turned = true;
                    }
                }
                if (!turned) {
                    // Last resort: try reverse direction
                    if (canMove(g->getX(), g->getY(), opposite)) {
                        g->turn(opposite);
                    } else {
                        // All directions blocked at pixel level — try any valid direction
                        for (int i = 0; i < 4; i++) {
                            Direction tryDir = static_cast<Direction>(i);
                            if (canMove(g->getX(), g->getY(), tryDir)) {
                                g->turn(tryDir);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            // Fallback: If target is unreachable or pixel-blocked, try Pacman
            if (scatterMode &&
                findPathTo(ghostR, ghostC, pacmanR, pacmanC, nextMove) &&
                canMove(g->getX(), g->getY(), nextMove)) {
                if (nextMove != g->_direction) {
                    g->turn(nextMove);
                }
            } else {
                Direction immediateNextDir = g->_direction;
                int nextR = ghostR + (immediateNextDir == Direction::UP ? -1 : (immediateNextDir == Direction::DOWN ? 1 : 0));
                int nextC = ghostC + (immediateNextDir == Direction::LEFT ? -1 : (immediateNextDir == Direction::RIGHT ? 1 : 0));
                if (!isValid(nextR, nextC)) {
                    int startIdx = random(4);
                    bool turned = false;
                    for (int i = 0; i < 4 && !turned; i++) {
                        Direction tryDir = static_cast<Direction>((startIdx + i) % 4);
                        int tryR = ghostR + (tryDir == Direction::UP ? -1 : (tryDir == Direction::DOWN ? 1 : 0));
                        int tryC = ghostC + (tryDir == Direction::LEFT ? -1 : (tryDir == Direction::RIGHT ? 1 : 0));
                        if (isValid(tryR, tryC) && canMove(g->getX(), g->getY(), tryDir)) {
                            g->turn(tryDir);
                            turned = true;
                        }
                    }
                    if (!turned) {
                        g->turn(oppositeDirection(g->_direction));
                    }
                }
            }
        }
    }
}

// Pinky's ambush target: 4 tiles ahead of Pacman's facing direction (task 1.4)
void Clockface::pinkyAmbushTarget(int pacmanR, int pacmanC, Direction pacmanDir, int& targetR, int& targetC) {
    bool foundForwardTile = false;
    for (int step = 1; step <= 4; step++) {
        int tryR = pacmanR + DIR_OFFSETS[static_cast<int>(pacmanDir)].dRow * step;
        int tryC = pacmanC + DIR_OFFSETS[static_cast<int>(pacmanDir)].dCol * step;
        if (tryR < 0 || tryR >= MAP_SIZE || tryC < 0 || tryC >= MAP_SIZE ||
            !isValid(tryR, tryC)) {
            break;
        }

        // Every earlier tile was traversable, so this is reachable without
        // targeting through a wall. Keep the farthest tile, up to four ahead.
        targetR = tryR;
        targetC = tryC;
        foundForwardTile = true;
    }
    if (foundForwardTile) {
        return;
    }
    // All forward tiles invalid — try perpendicular offsets (2-3 tiles flanking)
    // to achieve flanking behavior distinct from Blinky
    int perpOffsetsRow[2] = {0, 0};
    int perpOffsetsCol[2] = {0, 0};

    if (pacmanDir == Direction::RIGHT || pacmanDir == Direction::LEFT) {
        // Perpendicular = UP/DOWN
        perpOffsetsRow[0] = -2; perpOffsetsCol[0] = 0; // 2 tiles up
        perpOffsetsRow[1] = 2;  perpOffsetsCol[1] = 0; // 2 tiles down
    } else { // UP or DOWN
        // Perpendicular = LEFT/RIGHT
        perpOffsetsRow[0] = 0;  perpOffsetsCol[0] = -2; // 2 tiles left
        perpOffsetsRow[1] = 0;  perpOffsetsCol[1] = 2;  // 2 tiles right
    }

    for (int i = 0; i < 2; i++) {
        int tryR = pacmanR + perpOffsetsRow[i];
        int tryC = pacmanC + perpOffsetsCol[i];
        if (tryR >= 0 && tryR < MAP_SIZE && tryC >= 0 && tryC < MAP_SIZE && isValid(tryR, tryC)) {
            targetR = tryR;
            targetC = tryC;
            return;
        }
    }

    // Fallback: target Pacman's position directly
    targetR = pacmanR;
    targetC = pacmanC;
}
