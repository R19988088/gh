import Foundation
import CoreText
import CoreGraphics

// Input: Unicode code points; output: one fixed grayscale cell per code point.
let args = CommandLine.arguments
let codes = try String(contentsOfFile: args[1], encoding: .utf8).split(separator: "\n").map { UInt32($0)! }
let height = Int(args[3])!
let width = height - 2
let font = CTFontCreateWithName("PingFangSC-Regular" as CFString, CGFloat(width) * 0.9, nil)
var pixels = [UInt8](repeating: 0, count: width * height)
let output = FileHandle(forWritingAtPath: args[2])!
for code in codes {
    let char = UniChar(code)
    var input = char
    var glyph: CGGlyph = 0
    let string = String(UnicodeScalar(code)!) as CFString
    let glyphFont = CTFontCreateForString(font, string, CFRange(location: 0, length: 1))
    guard CTFontGetGlyphsForCharacters(glyphFont, &input, &glyph, 1), glyph != 0 else {
        fatalError(String(format: "System font does not cover U+%04X", code))
    }
    pixels.withUnsafeMutableBytes { bytes in
        bytes.initializeMemory(as: UInt8.self, repeating: 0)
        let context = CGContext(data: bytes.baseAddress, width: width, height: height,
            bitsPerComponent: 8, bytesPerRow: width, space: CGColorSpaceCreateDeviceGray(), bitmapInfo: 0)!
        context.setFillColor(gray: 1, alpha: 1)
        var advance = CGSize.zero
        CTFontGetAdvancesForGlyphs(glyphFont, .horizontal, &glyph, &advance, 1)
        let ascent = CTFontGetAscent(glyphFont)
        let descent = CTFontGetDescent(glyphFont)
        var point = CGPoint(x: (CGFloat(width) - advance.width) / 2,
                            y: (CGFloat(height) - ascent - descent) / 2 + descent)
        CTFontDrawGlyphs(glyphFont, &glyph, &point, 1, context)
    }
    output.write(Data(pixels))
}
try output.close()
