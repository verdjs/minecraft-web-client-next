import * as THREE from 'three'
import { beforeRenderFrame } from './beforeRenderFrame'
import type { WorldRendererThree } from 'minecraft-renderer/src/three/worldRendererThree'

interface MobAnimState {
  lastX: number
  lastY: number
  lastZ: number
  limbSwing: number
  limbSwingAmount: number
  lastTime: number
}

const mobAnimStates = new Map<number | string, MobAnimState>()

export function initMobAnimations () {
  beforeRenderFrame.push(updateMobAnimations)
}

function updateMobAnimations () {
  const world = (globalThis as any).world as WorldRendererThree | undefined
  const entitiesObj = world?.entities?.entities
  if (!entitiesObj) return

  const now = performance.now()

  for (const [id, entityMesh] of Object.entries(entitiesObj)) {
    if (!entityMesh) continue
    const orig = (entityMesh as any).originalEntity
    if (!orig || orig.type === 'player' || orig.type === 'object') continue

    let state = mobAnimStates.get(id)
    if (!state) {
      state = {
        lastX: entityMesh.position.x,
        lastY: entityMesh.position.y,
        lastZ: entityMesh.position.z,
        limbSwing: 0,
        limbSwingAmount: 0,
        lastTime: now,
      }
      mobAnimStates.set(id, state)
      continue
    }

    const dt = Math.max(0.001, (now - state.lastTime) / 1000)
    state.lastTime = now

    const dx = entityMesh.position.x - state.lastX
    const dz = entityMesh.position.z - state.lastZ
    state.lastX = entityMesh.position.x
    state.lastY = entityMesh.position.y
    state.lastZ = entityMesh.position.z

    const horizontalDist = Math.hypot(dx, dz)
    // Calculate speed in blocks per second
    const targetSpeed = Math.min(horizontalDist / dt, 6.0)

    // Smooth limbSwingAmount
    state.limbSwingAmount += (targetSpeed - state.limbSwingAmount) * Math.min(1.0, dt * 10)
    if (state.limbSwingAmount > 0.01) {
      state.limbSwing += horizontalDist * 4.0
    }

    // Find bones in entity mesh
    const bones: Record<string, THREE.Bone> = {}
    entityMesh.traverse((child: any) => {
      if (child.isBone && child.name) {
        if (!child.userData.bindRot) {
          child.userData.bindRot = {
            x: child.rotation.x,
            y: child.rotation.y,
            z: child.rotation.z
          }
        }
        const cleanName = child.name.replace(/^bone_/, '')
        bones[cleanName] = child
        bones[child.name] = child
      }
    })

    const leg0 = bones['leg0'] ?? bones['right_leg'] ?? bones['leg_back_right']
    const leg1 = bones['leg1'] ?? bones['left_leg'] ?? bones['leg_back_left']
    const leg2 = bones['leg2'] ?? bones['leg_front_right']
    const leg3 = bones['leg3'] ?? bones['leg_front_left']

    const arm0 = bones['arm0'] ?? bones['right_arm']
    const arm1 = bones['arm1'] ?? bones['left_arm']

    if (amount <= 0.005) {
      // Reset bones to initial bind pose when stopped
      if (leg0?.userData.bindRot) leg0.rotation.x = leg0.userData.bindRot.x
      if (leg1?.userData.bindRot) leg1.rotation.x = leg1.userData.bindRot.x
      if (leg2?.userData.bindRot) leg2.rotation.x = leg2.userData.bindRot.x
      if (leg3?.userData.bindRot) leg3.rotation.x = leg3.userData.bindRot.x
      if (arm0?.userData.bindRot) arm0.rotation.x = arm0.userData.bindRot.x
      if (arm1?.userData.bindRot) arm1.rotation.x = arm1.userData.bindRot.x
      continue
    }

    const isZombie = orig.name?.includes('zombie') || orig.name?.includes('husk') || orig.name?.includes('drowned')

    // Biped / Quadruped leg walking cycles relative to bind pose
    const wave = Math.cos(swing * 0.6662) * 1.2 * amount
    const waveOpposite = Math.cos(swing * 0.6662 + Math.PI) * 1.2 * amount

    if (leg0?.userData.bindRot) leg0.rotation.x = leg0.userData.bindRot.x + wave
    if (leg1?.userData.bindRot) leg1.rotation.x = leg1.userData.bindRot.x + waveOpposite
    if (leg2?.userData.bindRot) leg2.rotation.x = leg2.userData.bindRot.x + waveOpposite
    if (leg3?.userData.bindRot) leg3.rotation.x = leg3.userData.bindRot.x + wave

    if (isZombie) {
      // Zombie arms outstretched forward with subtle oscillation
      if (arm0?.userData.bindRot) arm0.rotation.x = -Math.PI / 2 + Math.sin(swing * 0.3) * 0.1 * amount
      if (arm1?.userData.bindRot) arm1.rotation.x = -Math.PI / 2 - Math.sin(swing * 0.3) * 0.1 * amount
    } else {
      // Standard biped arm swings relative to bind pose
      if (arm0?.userData.bindRot) arm0.rotation.x = arm0.userData.bindRot.x + waveOpposite * 0.8
      if (arm1?.userData.bindRot) arm1.rotation.x = arm1.userData.bindRot.x + wave * 0.8
    }
  }

  // Cleanup gone entities
  if (mobAnimStates.size > 200) {
    for (const id of mobAnimStates.keys()) {
      if (!entitiesObj[id]) {
        mobAnimStates.delete(id)
      }
    }
  }
}
