// this should actually be moved to mineflayer / renderer

import { fromFormattedString, TextComponent } from '@xmcl/text-component'
import type { IndexedData } from 'minecraft-data'
import { versionToNumber } from 'minecraft-renderer/src/lib/utils'

import mojangson from 'mojangson'
import nbt from 'prismarine-nbt'

export interface MessageFormatOptions {
  doShadow?: boolean
}

export type MessageFormatPart = Pick<TextComponent, 'hoverEvent' | 'clickEvent'> & {
  text: string
  color?: string
  bold?: boolean
  italic?: boolean
  underlined?: boolean
  strikethrough?: boolean
  obfuscated?: boolean
}

type MessageInput = {
  text?: string
  translate?: string
  with?: Array<MessageInput | string>
  color?: string
  bold?: boolean
  italic?: boolean
  underlined?: boolean
  strikethrough?: boolean
  obfuscated?: boolean
  extra?: MessageInput[]
  json?: any
}

const global = globalThis as any

// todo move to sign-renderer, replace with prismarine-chat, fix mcData issue!
export const formatMessage = (message: MessageInput | any, mcData: IndexedData = global.loadedData) => {
  let msglist: MessageFormatPart[] = []

  const simplifyInput = (input: any) => {
    if (!input) return input
    if (typeof input === 'object' && (input.type === 'compound' || input.type === 'list')) {
      return nbt.simplify(input)
    }
    return input
  }

  const readMsg = (msg: MessageInput | any) => {
    if (!msg) return
    msg = simplifyInput(msg)
    if (Array.isArray(msg)) {
      for (const item of msg) {
        readMsg(item)
      }
      return
    }

    if (typeof msg === 'string') {
      const trimmed = msg.trim()
      if (trimmed.startsWith('{') || trimmed.startsWith('[')) {
        try {
          msg = mojangson.simplify(mojangson.parse(trimmed))
          if (Array.isArray(msg)) {
            for (const item of msg) readMsg(item)
            return
          }
        } catch {
          msg = { text: msg }
        }
      } else {
        msg = { text: msg }
      }
    }

    if (typeof msg === 'object' && (msg.type === 'compound' || msg.type === 'list')) {
      msg = nbt.simplify(msg)
    }

    const styles = {
      color: msg.color,
      bold: !!msg.bold,
      italic: !!msg.italic,
      underlined: !!msg.underlined,
      strikethrough: !!msg.strikethrough,
      obfuscated: !!msg.obfuscated
    }

    if (!msg.text && typeof msg.json?.[''] === 'string') msg.text = msg.json['']
    if (msg.text) {
      msglist.push({
        ...msg,
        text: String(msg.text),
        ...styles
      })
    } else if (msg.translate) {
      const tText = mcData?.language?.[msg.translate] ?? msg.translate

      if (msg.with) {
        const splitted = tText.split(/%s|%\d+\$s/g)

        let i = 0
        for (const [j, part] of splitted.entries()) {
          msglist.push({ text: part, ...styles })

          if (j + 1 < splitted.length) {
            if (msg.with[i]) {
              const msgWith = msg.with[i]
              if (typeof msgWith === 'string') {
                readMsg({
                  ...styles,
                  text: msgWith
                })
              } else {
                readMsg({
                  ...styles,
                  ...msgWith
                })
              }
            }
            i++
          }
        }
      } else {
        msglist.push({
          ...msg,
          text: tText,
          ...styles
        })
      }
    }

    if (msg.extra) {
      const extraList = Array.isArray(msg.extra) ? msg.extra : [msg.extra]
      for (let ex of extraList) {
        if (typeof ex === 'string') {
          ex = { text: ex }
        }
        readMsg({ ...styles, ...simplifyInput(ex) })
      }
    }
  }

  readMsg(message)

  const flat = (msg) => {
    return [msg, msg.extra?.flatMap(flat) ?? []]
  }

  msglist = msglist.map(msg => {
    // normalize §
    if (typeof msg.text !== 'string' || !msg.text.includes?.('§')) return msg
    const newMsg = fromFormattedString(msg.text)
    return flat(newMsg)
  }).flat(Infinity)

  return msglist
}

export const messageToString = (message: MessageInput | string) => {
  if (typeof message === 'string') {
    return message
  }
  const msglist = formatMessage(message)
  return msglist.map(msg => msg.text).join('')
}

const COLOR_TO_SECTION_CODE: Record<string, string> = {
  black: '§0',
  dark_blue: '§1',
  dark_green: '§2',
  dark_aqua: '§3',
  dark_red: '§4',
  dark_purple: '§5',
  gold: '§6',
  gray: '§7',
  dark_gray: '§8',
  blue: '§9',
  green: '§a',
  aqua: '§b',
  red: '§c',
  light_purple: '§d',
  yellow: '§e',
  white: '§f',
}

export function partsToFormattedSectionString (parts: MessageFormatPart[]): string {
  let out = ''
  for (const part of parts) {
    if (!part.text && part.text !== '') continue
    let prefix = ''
    if (part.color) {
      const lower = String(part.color).toLowerCase().trim()
      if (COLOR_TO_SECTION_CODE[lower]) {
        prefix += COLOR_TO_SECTION_CODE[lower]
      } else if (lower === '#00ff00' || lower === '#55ff55' || lower === '#00aa00' || lower.includes('green')) {
        prefix += '§a'
      } else if (lower === '#ffffff' || lower.includes('white')) {
        prefix += '§f'
      } else if (lower === '#555555' || lower === '#333333' || lower.includes('dark_gray') || lower.includes('dark-gray')) {
        prefix += '§8'
      } else if (lower === '#aaaaaa' || lower.includes('gray')) {
        prefix += '§7'
      } else if (lower === '#ffaa00' || lower.includes('gold')) {
        prefix += '§6'
      } else if (lower === '#ff5555' || lower.includes('red')) {
        prefix += '§c'
      } else if (lower === '#55ffff' || lower.includes('aqua')) {
        prefix += '§b'
      } else if (lower === '#5555ff' || lower.includes('blue')) {
        prefix += '§9'
      } else if (lower === '#ffff55' || lower.includes('yellow')) {
        prefix += '§e'
      } else if (lower === '#ff55ff' || lower.includes('purple')) {
        prefix += '§d'
      }
    }
    if (part.bold) prefix += '§l'
    if (part.italic) prefix += '§o'
    if (part.underlined) prefix += '§n'
    if (part.strikethrough) prefix += '§m'
    if (part.obfuscated) prefix += '§k'
    out += prefix + part.text
  }
  return out
}

export function formatLoreLineToString (line: any): string {
  if (line === null || line === undefined) return ''
  const parts = formatMessage(line)
  if (!parts || parts.length === 0) {
    if (typeof line === 'string') {
      if (/^\s*\$|value|price|coins|worth/i.test(line) && !line.includes('§')) {
        return `§a${line}`
      }
      return line
    }
    return ''
  }
  const formatted = partsToFormattedSectionString(parts)
  if (/^\s*\$|value|price|coins|worth/i.test(formatted) && !formatted.includes('§')) {
    return `§a${formatted}`
  }
  return formatted || parts.map(p => p.text).join('')
}

const blockToItemRemaps = {
  water: 'water_bucket',
  lava: 'lava_bucket',
  redstone_wire: 'redstone',
  tripwire: 'tripwire_hook'
}

export const getItemFromBlock = (block: import('prismarine-block').Block) => {
  const item = global.loadedData.itemsByName[blockToItemRemaps[block.name] ?? block.name]
  return item
}

export function isAllowedChatCharacter (char: string): boolean {
  // if (char.length !== 1) {
  //   throw new Error('Input must be a single character')
  // }

  const charCode = char.codePointAt(0)!
  return charCode !== 167 && charCode >= 32 && charCode !== 127
}

export const isStringAllowed = (str: string) => {
  const invalidChars = new Set<string>()
  for (const [i, char] of [...str].entries()) {
    const isSurrogatePair = str.codePointAt(i) !== str['charCodeAt'](i)
    if (isSurrogatePair) continue

    if (!isAllowedChatCharacter(char)) {
      invalidChars.add(char)
    }
  }

  const valid = invalidChars.size === 0
  if (valid) {
    return {
      valid: true
    }
  }

  return {
    valid,
    clean: [...str].filter(c => !invalidChars.has(c)).join(''),
    invalid: [...invalidChars]
  }
}
