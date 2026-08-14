import { describe, expect, it } from 'vitest'
import { getItemMetadata, getItemNameRaw } from './items'

describe('getItemMetadata and getItemNameRaw', () => {
  it('extracts standard item displayName without garbling when lore is present', () => {
    const mockItem = {
      name: 'cooked_beef',
      displayName: 'Steak',
      components: [
        {
          type: 'lore',
          data: [
            {
              type: 'compound',
              value: {
                extra: {
                  type: 'list',
                  value: {
                    type: 'compound',
                    value: [
                      { color: { type: 'string', value: '#00FF00' }, text: { type: 'string', value: '$ ' } },
                      { color: { type: 'string', value: 'white' }, text: { type: 'string', value: '6.6K' } }
                    ]
                  }
                },
                text: { type: 'string', value: '' }
              }
            }
          ]
        }
      ]
    }

    const metadata = getItemMetadata(mockItem as any, {} as any)
    expect(metadata.customText).toBeUndefined()
    expect(metadata.lore).toBeDefined()
    expect(metadata.lore?.length).toBe(1)

    const rawName = getItemNameRaw(mockItem as any, {} as any)
    expect(rawName).toBeUndefined()
  })

  it('correctly handles custom_name component', () => {
    const mockItem = {
      name: 'diamond_sword',
      components: [
        {
          type: 'custom_name',
          data: '{"text":"Excalibur","color":"gold"}'
        }
      ]
    }

    const metadata = getItemMetadata(mockItem as any, {} as any)
    expect(metadata.customText).toBe('{"text":"Excalibur","color":"gold"}')

    const rawName = getItemNameRaw(mockItem as any, {} as any)
    expect(rawName).toEqual({
      text: 'Excalibur',
      color: 'gold'
    })
  })

  it('correctly handles legacy NBT display Name', () => {
    const mockItem = {
      name: 'stick',
      nbt: {
        type: 'compound',
        value: {
          display: {
            type: 'compound',
            value: {
              Name: {
                type: 'string',
                value: '{"text":"Magic Wand"}'
              }
            }
          }
        }
      }
    }

    const metadata = getItemMetadata(mockItem as any, {} as any)
    expect(metadata.customText).toBe('{"text":"Magic Wand"}')

    const rawName = getItemNameRaw(mockItem as any, {} as any)
    expect(rawName).toEqual({
      text: 'Magic Wand'
    })
  })

  it('correctly extracts legacy NBT Lore and formats price in green', () => {
    const mockItem = {
      name: 'cooked_beef',
      nbt: {
        type: 'compound',
        value: {
          display: {
            type: 'compound',
            value: {
              Lore: {
                type: 'list',
                value: {
                  type: 'string',
                  value: ['$ 12.5K', 'Freshly grilled']
                }
              }
            }
          }
        }
      }
    }

    const metadata = getItemMetadata(mockItem as any, {} as any)
    expect(metadata.lore).toBeDefined()
    expect(metadata.lore?.length).toBe(2)
    expect(metadata.lore?.[0]).toEqual({
      text: '$ 12.5K',
      color: '#55FF55'
    })
    expect(metadata.lore?.[1]).toEqual({
      text: 'Freshly grilled'
    })
  })
})

