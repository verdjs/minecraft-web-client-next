import { Vec3 } from 'vec3'
import { subscribe } from 'valtio'
import {
  AppViewer,
  getInitialPlayerState,
  MENU_BACKGROUND_MC_VERSION,
  menuBackgroundOptionsFromStorage,
  type MenuBackgroundOptions
} from 'minecraft-renderer/src'
import { generateGuiAtlas } from 'minecraft-renderer/src/lib/guiRenderer'
import { BotEvents, type EntityMovedMetadata } from 'mineflayer'
import { activeModalStack, miscUiState } from './globalState'
import { options } from './optionsStorage'
import { watchOptionsAfterWorldViewInit } from './watchOptions'
import { updateLightRemeshBlockKey } from './mineflayer/updateLightRemeshKey'
import { buildEntityRenderHints } from './boatRenderHints'

// do not import this. Use global appViewer instead (without window prefix).
export const appViewer = new AppViewer()
appViewer.resourcesManager.generateGuiTextures = async () => {
  await generateGuiAtlas(appViewer)
}
window.appViewer = appViewer

appViewer.onWorldStart = () => {
  connectAppWorldViewToBot()

  if (appViewer.worldView) {
    watchOptionsAfterWorldViewInit(appViewer.worldView)
  }
}

const prepareMenuBackgroundAssets = async (opts: MenuBackgroundOptions) => {
  if (!opts.useMinecraftTextures) return
  const { loadMinecraftData } = await import('./connect')
  await loadMinecraftData(MENU_BACKGROUND_MC_VERSION)
  appViewer.resourcesManager.currentConfig = {
    version: MENU_BACKGROUND_MC_VERSION,
    texturesVersion: options.useVersionsTextures || undefined,
    noInventoryGui: true
  }
  await appViewer.resourcesManager.updateAssetsData({})
}

const initialMenuStart = async () => {
  if (appViewer.currentDisplay === 'world') {
    appViewer.resetBackend(true)
  }
  const demo = new URLSearchParams(window.location.search).get('demo')
  if (!demo) {
    const menuBackgroundOpts = menuBackgroundOptionsFromStorage(options)
    await prepareMenuBackgroundAssets(menuBackgroundOpts)
    appViewer.startMenuBackground(menuBackgroundOpts)
    return
  }

  // const version = '1.18.2'
  const version = '1.21.4'
  const { loadMinecraftData } = await import('./connect')
  const { getSyncWorld } = await import('minecraft-renderer/src/playground/shared')
  await loadMinecraftData(version)
  const world = getSyncWorld(version)
  world.setBlockStateId(new Vec3(0, 64, 0), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(1, 64, 0), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(1, 64, 1), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(0, 64, 1), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(-1, 64, -1), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(-1, 64, 0), loadedData.blocksByName.water.defaultState)
  world.setBlockStateId(new Vec3(0, 64, -1), loadedData.blocksByName.water.defaultState)
  appViewer.resourcesManager.currentConfig = { version }
  appViewer.playerState.reactive = getInitialPlayerState()
  await appViewer.resourcesManager.updateAssetsData({})
  await appViewer.startWorld(world, 3)
  if (appViewer.worldView) watchOptionsAfterWorldViewInit(appViewer.worldView)
  appViewer.backend!.updateCamera(new Vec3(0, 65.7, 0), 0, -Math.PI / 2) // Y+1 and pitch = PI/2 to look down
  void appViewer.worldView!.init(new Vec3(0, 64, 0))
}
window.initialMenuStart = initialMenuStart

const hasAppStatus = () => activeModalStack.some(m => m.reactType === 'app-status')

export const onAppViewerConfigUpdate = () => {
  appViewer.inWorldRenderingConfig.skinTexturesProxy = miscUiState.appConfig?.skinTexturesProxy
}

export const modalStackUpdateChecks = () => {
  if (!miscUiState.gameLoaded && !hasAppStatus()) {
    void initialMenuStart()
  }

  if (appViewer.backend) {
    appViewer.backend.setRendering(!hasAppStatus())
  }

  appViewer.inWorldRenderingConfig.foreground = activeModalStack.length === 0
}
subscribe(activeModalStack, modalStackUpdateChecks)

/** Chunks that received `update_light` before worldView finished the initial load. */
const pendingUpdateLightRelight = new Set<string>()

interface ChunkLoadTask {
  pos: Vec3
  isLightUpdate?: boolean
  reason: string
}

const pendingChunkLoadMap = new Map<string, ChunkLoadTask>()
let isChunkQueueScheduled = false

const processChunkLoadQueue = () => {
  if (pendingChunkLoadMap.size === 0) {
    isChunkQueueScheduled = false
    return
  }

  const botPos = (globalThis as any).bot?.entity?.position
  const isGameLoaded = miscUiState.gameLoaded

  // Sort tasks: load nearest chunks to player first
  const tasks = Array.from(pendingChunkLoadMap.values())
  if (botPos && tasks.length > 1) {
    tasks.sort((a, b) => {
      const distA = Math.hypot(a.pos.x - botPos.x, a.pos.z - botPos.z)
      const distB = Math.hypot(b.pos.x - botPos.x, b.pos.z - botPos.z)
      return distA - distB
    })
  }

  const startTime = performance.now()
  // Responsive budget: up to 6 chunks or 3.5ms per frame to render fast without clear holes or fps drops
  const MAX_FRAME_BUDGET_MS = isGameLoaded ? 3.5 : 25.0
  const MAX_CHUNKS_PER_BATCH = isGameLoaded ? 6 : 50
  let processed = 0

  for (const task of tasks) {
    const key = `${task.pos.x},${task.pos.z}`
    pendingChunkLoadMap.delete(key)
    void appViewer.worldView?.loadChunk(task.pos, task.isLightUpdate ?? false, task.reason)
    processed++
    if (processed >= MAX_CHUNKS_PER_BATCH || performance.now() - startTime >= MAX_FRAME_BUDGET_MS) {
      break
    }
  }

  if (pendingChunkLoadMap.size > 0) {
    requestAnimationFrame(processChunkLoadQueue)
  } else {
    isChunkQueueScheduled = false
  }
}

const enqueueChunkLoad = (pos: Vec3, isLightUpdate = false, reason = 'chunkColumnLoad') => {
  const key = `${pos.x},${pos.z}`
  const existing = pendingChunkLoadMap.get(key)
  const keepLight = existing ? (existing.isLightUpdate && isLightUpdate) : isLightUpdate
  pendingChunkLoadMap.set(key, { pos, isLightUpdate: keepLight, reason })
  if (!isChunkQueueScheduled) {
    isChunkQueueScheduled = true
    requestAnimationFrame(processChunkLoadQueue)
  }
}

const connectAppWorldViewToBot = () => {
  const entitiesObjectData = new Map<string, number>()
  const deadEntities = new Set<number>()
  bot._client.prependListener('spawn_entity', (data) => {
    if (data.objectData && data.entityId !== undefined) {
      entitiesObjectData.set(data.entityId, data.objectData)
    }
  })

  const emitEntity = (e, name = 'entity', eventMetadata = {}) => {
    if (!e) return
    if (e === bot.entity) {
      if (name === 'entity') {
        appViewer.worldView?.emit('playerEntity', e)
      }
      return
    }
    if (!e.name) return // mineflayer received update for not spawned entity
    if (deadEntities.has(e.id)) return
    e.objectData = entitiesObjectData.get(e.id)
    const renderHints = buildEntityRenderHints(e, {
      localVehicle: bot.vehicle,
      localBoatStatus: bot._boatPhysics?.getStatus?.() ?? null,
      localBoatPaddleState: bot._boatPhysics?.getPaddleState?.() ?? null,
      horseControllerActive: Boolean(bot._horsePhysics?.getCtx?.()),
      world: bot.world,
      waterIds: {
        waterId: bot.registry.blocksByName.water.id,
        flowingWaterId: bot.registry.blocksByName.flowing_water?.id,
      },
      version: bot.version,
      entityMetadataKeys: bot.registry.entitiesByName[e.name]?.metadataKeys,
    })
    appViewer.worldView?.emit(name as any, {
      ...e,
      ...eventMetadata,
      pos: e.position,
      username: e.username,
      team: bot.teamMap[e.username] || bot.teamMap[e.uuid],
      renderHints,
    })
  }

  const pendingPassengerVehicles = new Map<number, any>()
  let passengerVehicleFlushScheduled = false
  const queuePassengerVehicleRefresh = (vehicle: any) => {
    if (!vehicle) return
    pendingPassengerVehicles.set(vehicle.id, vehicle)
    if (passengerVehicleFlushScheduled) return
    passengerVehicleFlushScheduled = true
    queueMicrotask(() => {
      passengerVehicleFlushScheduled = false
      const vehicles = [...pendingPassengerVehicles.values()]
      pendingPassengerVehicles.clear()
      for (const pendingVehicle of vehicles) {
        emitEntity(pendingVehicle)
      }
    })
  }

  const eventListeners = {
    entitySpawn (e: any) {
      if (e.name === 'item_frame' || e.name === 'glow_item_frame') {
        e.position.translate(0.5, 0.5, 0.5)
      }
      emitEntity(e)
    },
    entityUpdate (e: any) {
      emitEntity(e)
    },
    entityEquip (e: any) {
      emitEntity(e)
    },
    entityMoved (e: any, eventMetadata: EntityMovedMetadata = {}) {
      emitEntity(e, 'entityMoved', eventMetadata)
    },
    entityAttach (_passenger: any, vehicle: any) {
      queuePassengerVehicleRefresh(vehicle)
    },
    entityDetach (_passenger: any, vehicle: any) {
      queuePassengerVehicleRefresh(vehicle)
    },
    entityDead (e: any) {
      if (e === bot.entity) return
      if (deadEntities.has(e.id)) return
      deadEntities.add(e.id)
      appViewer.worldView?.emit('entity', { id: e.id, delete: true })
    },
    entityGone (e: any) {
      deadEntities.delete(e.id)
      appViewer.worldView?.emit('entity', { id: e.id, delete: true })
    },
    chunkColumnLoad (pos: Vec3) {
      const now = performance.now()
      if (appViewer.worldView?.lastChunkReceiveTime) {
        appViewer.worldView.chunkReceiveTimes.push(now - appViewer.worldView.lastChunkReceiveTime)
      }
      appViewer.worldView!.lastChunkReceiveTime = now

      if (appViewer.worldView?.waitingSpiralChunksLoad[`${pos.x},${pos.z}`]) {
        appViewer.worldView?.waitingSpiralChunksLoad[`${pos.x},${pos.z}`](true)
        delete appViewer.worldView?.waitingSpiralChunksLoad[`${pos.x},${pos.z}`]
      } else if (appViewer.worldView?.loadedChunks[`${pos.x},${pos.z}`]) {
        enqueueChunkLoad(pos, false, 'Received another chunkColumnLoad event while already loaded')
      } else {
        enqueueChunkLoad(pos, false, 'chunkColumnLoad')
      }
      appViewer.worldView?.chunkProgress()
    },
    chunkColumnUnload (pos: Vec3) {
      pendingChunkLoadMap.delete(`${pos.x},${pos.z}`)
      appViewer.worldView?.unloadChunk(pos)
    },
    blockUpdate (oldBlock: any, newBlock: any) {
      const stateId = newBlock.stateId ?? ((newBlock.type << 4) | newBlock.metadata)
      appViewer.worldView?.emit('blockUpdate', { pos: oldBlock.position, stateId })
    },
    time () {
      appViewer.worldView?.emit('time', bot.time.timeOfDay)
    },
    end () {
      appViewer.worldView?.emit('end')
    },
    login () {
      void appViewer.worldView?.updatePosition(bot.entity.position, true)
      appViewer.worldView?.emit('playerEntity', bot.entity)
    },
    respawn () {
      void appViewer.worldView?.updatePosition(bot.entity.position, true)
      appViewer.worldView?.emit('playerEntity', bot.entity)
      appViewer.worldView?.emit('onWorldSwitch')
    },
  } satisfies Partial<BotEvents>


  appViewer.worldView?.on('loadChunk', (data) => {
    if (data.isLightUpdate) return
    const key = `${data.x},${data.z}`
    if (!pendingUpdateLightRelight.delete(key)) return
    enqueueChunkLoad(new Vec3(data.x, 0, data.z), true, 'update_light-pending')
  })

  bot._client.on('update_light', ({ chunkX, chunkZ }) => {
    const key = updateLightRemeshBlockKey(chunkX, chunkZ)
    const bx = chunkX * 16
    const bz = chunkZ * 16
    const waiting = !!appViewer.worldView?.waitingSpiralChunksLoad[key]
    const loaded = !!appViewer.worldView?.loadedChunks[key]
    if (!waiting && loaded) {
      enqueueChunkLoad(new Vec3(bx, 0, bz), true, 'update_light')
    } else if (!loaded) {
      pendingUpdateLightRelight.add(key)
    }
  })

  for (const [evt, listener] of Object.entries(eventListeners)) {
    bot.on(evt as any, listener)
  }

  // eslint-disable-next-line guard-for-in
  for (const id in bot.entities) {
    const e = bot.entities[id]
    try {
      emitEntity(e)
    } catch (err) {
      console.error('error processing entity', err)
    }
  }
}
