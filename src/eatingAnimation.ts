import * as THREE from 'three'
import { beforeRenderFrame } from './beforeRenderFrame'
import { playerState } from './mineflayer/playerState'
import type { WorldRendererThree } from 'minecraft-renderer/src/three/worldRendererThree'

export function initEatingAnimation () {
  beforeRenderFrame.push(updateEatingAnimation)
}

function updateEatingAnimation () {
  const world = (globalThis as any).world as WorldRendererThree | undefined
  const holdingBlockController = world?.holdingBlock
  const holdingMesh = holdingBlockController?.holdingBlock

  if (!holdingMesh) {
    return
  }

  const isUsing = playerState.isUsingItem && holdingBlockController?.currentDisplayType !== 'hand'
  const usageTicks = playerState.reactive.itemUsageTicks ?? 0

  if (isUsing && usageTicks > 0) {
    // 32 ticks standard use duration (1.6s)
    const progress = Math.min(usageTicks / 32, 1.0)
    // Smooth ease-in for bringing item to mouth
    const mouthFactor = 1.0 - Math.pow(1.0 - Math.min(progress * 2.0, 1.0), 3)

    // Vanilla rhythmic chew oscillation while eating (until 85% finished)
    const chewCycle = (usageTicks % 4) / 4
    const chew = progress < 0.85 ? Math.abs(Math.sin(chewCycle * Math.PI)) * 0.06 : 0

    // Apply offset directly to holdingMesh position/rotation
    holdingMesh.position.x = 0.08 * mouthFactor
    holdingMesh.position.y = (0.12 + chew) * mouthFactor
    holdingMesh.position.z = 0.18 * mouthFactor

    holdingMesh.rotation.x = THREE.MathUtils.degToRad(35 * mouthFactor)
    holdingMesh.rotation.y = THREE.MathUtils.degToRad(-20 * mouthFactor)
    holdingMesh.rotation.z = THREE.MathUtils.degToRad(15 * mouthFactor + (chew * 80))
  } else if (holdingMesh.position.x !== 0 || holdingMesh.position.y !== 0 || holdingMesh.position.z !== 0) {
    // Reset when not using
    holdingMesh.position.set(0, 0, 0)
    holdingMesh.rotation.set(0, 0, 0)
  }
}
