import mojangson from 'mojangson'
import nbt from 'prismarine-nbt'
import { fromFormattedString } from '@xmcl/text-component'
import { getItemSelector, PlayerStateRenderer } from 'minecraft-renderer/src/playerState/playerState'
import { getItemDefinition } from 'mc-assets/dist/itemDefinitions'
import { ItemSpecificContextProperties } from 'minecraft-renderer/src/playerState/types'
import { ResourcesManagerCommon } from 'minecraft-renderer/src/resourcesManager'
import { MessageFormatPart } from '../chatUtils'

type RenderSlotComponent = {
  type: string,
  data: any
  // example
  // {
  //   "type": "item_model",
  //   "data": "aa:ss"
  // }
}
export type RenderItem = Pick<import('prismarine-item').Item, 'name' | 'displayName' | 'durabilityUsed' | 'maxDurability' | 'enchants' | 'nbt'> & {
  components?: RenderSlotComponent[],
  // componentMap?: Map<string, RenderSlotComponent>
}
export type GeneralInputItem = Pick<import('prismarine-item').Item, 'name' | 'nbt'> & {
  components?: RenderSlotComponent[],
  displayName?: string
  modelResolved?: boolean
}

type JsonString = string
type PossibleItemProps = {
  CustomModelData?: number
  Damage?: number
  display?: { Name?: JsonString } // {"text":"Knife","color":"white","italic":"true"}
}

const formatLoreLine = (line: any): any => {
  if (line === null || line === undefined) return undefined
  if (typeof line === 'object') {
    if (line.type === 'compound' || line.type === 'list') {
      return nbt.simplify(line)
    }
    return line
  }
  if (typeof line === 'string') {
    try {
      const trimmed = line.trim()
      if ((trimmed.startsWith('{') && trimmed.endsWith('}')) || (trimmed.startsWith('[') && trimmed.endsWith(']'))) {
        let parsed = mojangson.parse(trimmed)
        if (parsed && typeof parsed === 'object' && (parsed.type === 'compound' || parsed.type === 'list')) {
          parsed = nbt.simplify(parsed)
        } else {
          parsed = mojangson.simplify(parsed)
        }
        return parsed
      }
      if (trimmed.includes('§')) {
        return fromFormattedString(trimmed)
      }
      // Economy / price line without explicit formatting -> highlight in green
      if (/^\s*\$|value|price|coins|worth/i.test(trimmed)) {
        return { text: trimmed, color: '#55FF55' }
      }
      return { text: trimmed }
    } catch {
      return { text: String(line) }
    }
  }
  return { text: String(line) }
}

export const getItemMetadata = (item: GeneralInputItem, resourcesManager: ResourcesManagerCommon) => {
  let customText = undefined as string | any | undefined
  let customModel = undefined as string | undefined
  let lore = undefined as any[] | undefined

  let itemId = item.name
  if (!itemId.includes(':')) {
    itemId = `minecraft:${itemId}`
  }
  const customModelDataDefinitions = resourcesManager.currentResources?.customItemModelNames[itemId]

  if (item.components) {
    const componentMap = new Map<string, RenderSlotComponent>()
    for (const component of item.components) {
      if (!component?.type) continue
      const normalizedType = component.type.replace(/^minecraft:/, '')
      componentMap.set(normalizedType, component)
      componentMap.set(component.type, component)
    }

    const customTextComponent = componentMap.get('custom_name') || componentMap.get('item_name')
    if (customTextComponent) {
      if (typeof customTextComponent.data === 'string') {
        customText = customTextComponent.data
      } else if (customTextComponent.data?.type === 'compound' || customTextComponent.data?.type === 'list') {
        customText = nbt.simplify(customTextComponent.data)
      } else {
        customText = customTextComponent.data
      }
    }
    const customModelComponent = componentMap.get('item_model')
    if (customModelComponent) {
      customModel = customModelComponent.data
    }
    if (customModelDataDefinitions) {
      const customModelDataComponent: any = componentMap.get('custom_model_data')
      if (customModelDataComponent?.data) {
        let customModelData: number | undefined
        if (typeof customModelDataComponent.data === 'number') {
          customModelData = customModelDataComponent.data
        } else if (typeof customModelDataComponent.data === 'object'
          && 'floats' in customModelDataComponent.data
          && Array.isArray(customModelDataComponent.data.floats)
          && customModelDataComponent.data.floats.length > 0) {
          customModelData = customModelDataComponent.data.floats[0]
        }
        if (customModelData && customModelDataDefinitions[customModelData]) {
          customModel = customModelDataDefinitions[customModelData]
        }
      }
    }
    const loreComponent = componentMap.get('lore')
    if (loreComponent) {
      let rawLore = loreComponent.data
      if (rawLore?.type === 'list' || rawLore?.type === 'compound') {
        rawLore = nbt.simplify(rawLore)
      }
      if (rawLore?.lines) {
        rawLore = rawLore.lines
      }
      if (rawLore?.value) {
        rawLore = rawLore.value
      }
      if (Array.isArray(rawLore)) {
        lore = rawLore.map(formatLoreLine).filter(Boolean)
      }
    }
    const customDataComponent = componentMap.get('custom_data')
    if (customDataComponent?.data) {
      let customData = customDataComponent.data
      if (customData?.type === 'compound') {
        customData = nbt.simplify(customData)
      }
      if (typeof customData === 'object' && customData !== null) {
        if (!customText && (customData.display?.Name || customData.Name)) {
          customText = customData.display?.Name ?? customData.Name
        }
        if (!lore && (customData.display?.Lore || customData.Lore)) {
          const cLore = customData.display?.Lore ?? customData.Lore
          if (Array.isArray(cLore)) {
            lore = cLore.map(formatLoreLine).filter(Boolean)
          }
        }
      }
    }
  }
  if (item.nbt) {
    const itemNbt: PossibleItemProps & Record<string, any> = nbt.simplify(item.nbt)
    const customName = itemNbt.display?.Name
    if (customName) {
      customText = customName
    }
    if (!lore) {
      const nbtLore = itemNbt.display?.Lore ?? itemNbt.Lore
      if (Array.isArray(nbtLore)) {
        lore = nbtLore.map(formatLoreLine).filter(Boolean)
      }
    }
    if (customModelDataDefinitions && itemNbt.CustomModelData && customModelDataDefinitions[itemNbt.CustomModelData]) {
      customModel = customModelDataDefinitions[itemNbt.CustomModelData]
    }
  }

  return {
    customText,
    customModel,
    lore
  }
}

export const getItemLoreRaw = (item: Pick<import('prismarine-item').Item, 'nbt'> | null, resourcesManager: ResourcesManagerCommon): any[] => {
  if (!item) return []
  const { lore } = getItemMetadata(item as GeneralInputItem, resourcesManager)
  return lore ?? []
}


export const getItemNameRaw = (item: Pick<import('prismarine-item').Item, 'nbt'> | null, resourcesManager: ResourcesManagerCommon) => {
  if (!item) return ''
  const { customText } = getItemMetadata(item as GeneralInputItem, resourcesManager)
  if (!customText) return
  try {
    if (typeof customText === 'object') {
      return customText
    }
    const trimmed = String(customText).trim()
    if ((trimmed.startsWith('{') && trimmed.endsWith('}')) || (trimmed.startsWith('[') && trimmed.endsWith(']'))) {
      const parsed = mojangson.simplify(mojangson.parse(trimmed))
      if (typeof parsed === 'object' && parsed !== null) {
        return parsed
      }
      return fromFormattedString(String(parsed))
    }
    return fromFormattedString(trimmed)
  } catch (err) {
    return {
      text: String(customText)
    }
  }
}

export const getItemModelName = (item: GeneralInputItem, specificProps: ItemSpecificContextProperties, resourcesManager: ResourcesManagerCommon, playerState: PlayerStateRenderer) => {
  let itemModelName = item.name
  const { customModel } = getItemMetadata(item, resourcesManager)
  if (customModel) {
    itemModelName = customModel
  }

  const itemSelector = getItemSelector(playerState, {
    ...specificProps
  })
  const modelFromDef = getItemDefinition(appViewer.resourcesManager.currentResources!.itemsDefinitionsStore, {
    name: itemModelName,
    version: appViewer.resourcesManager.currentResources!.version,
    properties: itemSelector
  })?.model
  const model = (modelFromDef === 'minecraft:special' ? undefined : modelFromDef) ?? itemModelName
  return model
}
