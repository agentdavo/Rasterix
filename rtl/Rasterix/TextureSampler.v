// Rasterix
// https://github.com/ToNi3141/Rasterix
// Copyright (c) 2023 ToNi3141

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Gets texel coordinates and then samples a texel quad from the texture memory.
// It assumes that a memory with a fixed delay is used. The delay can be configured with MEMORY_DELAY
// It supports texture sizes from 1x1 to 256x256
// Delay: 3 clocks
module TextureSampler #(
    parameter MEMORY_DELAY = 1,
    parameter PIXEL_WIDTH = 32,
    localparam ADDR_WIDTH = 17 // Based on the maximum texture size, of 256x256 (8 bit x 8 bit) + mipmap levels in PIXEL_WIDTH word addresses
)
(
    input  wire                         aclk,
    input  wire                         resetn,

    // Texture size
    // textureSize * 2. 0 equals 1px. 1 equals 2px. 2 equals 4px... Only power of two are allowed.
    input  wire [ 3 : 0]                textureSizeWidth, 
    input  wire [ 3 : 0]                textureSizeHeight,
    input  wire [ 3 : 0]                textureLod,
    input  wire                         enableHalfPixelOffset,

    // Texture memory access of a texel quad
    output reg  [ADDR_WIDTH - 1 : 0]    texelAddr00,
    output reg  [ADDR_WIDTH - 1 : 0]    texelAddr01,
    output reg  [ADDR_WIDTH - 1 : 0]    texelAddr10,
    output reg  [ADDR_WIDTH - 1 : 0]    texelAddr11,
    input  wire [PIXEL_WIDTH - 1 : 0]   texelInput00,
    input  wire [PIXEL_WIDTH - 1 : 0]   texelInput01,
    input  wire [PIXEL_WIDTH - 1 : 0]   texelInput10,
    input  wire [PIXEL_WIDTH - 1 : 0]   texelInput11,

    // Texture Read
    input  wire [31 : 0]                texelS, // S16.15
    input  wire [31 : 0]                texelT, // S16.15
    input  wire                         clampS,
    input  wire                         clampT,
    output wire [PIXEL_WIDTH - 1 : 0]   texel00, // (0, 0), as (s, t). s and t are switched since the address is constructed like {texelT, texelS}
    output wire [PIXEL_WIDTH - 1 : 0]   texel01, // (1, 0)
    output wire [PIXEL_WIDTH - 1 : 0]   texel10, // (0, 1)
    output wire [PIXEL_WIDTH - 1 : 0]   texel11, // (1, 1)

    // This is basically the faction of the pixel coordinate and has a range from 0.0 (0x0) to 0.999... (0xffff)
    // The integer part is not required, since the integer part only adresses the pixel and we don't care about that.
    // We just care about the coordinates within the texel quad. And if there the coordinate gets >1.0, that means, we
    // are outside of our quad which never happens.
    output wire [15 : 0]                texelSubCoordS, // Q0.16
    output wire [15 : 0]                texelSubCoordT // Q0.16
);
`include "RegisterAndDescriptorDefines.vh"

    function [PIXEL_WIDTH - 1 : 0] texelMux;
        input [ 1 : 0] sel;
        input [PIXEL_WIDTH - 1 : 0] t00;
        input [PIXEL_WIDTH - 1 : 0] t01;
        input [PIXEL_WIDTH - 1 : 0] t10;
        input [PIXEL_WIDTH - 1 : 0] t11;
        begin
            case (sel)
                2'd0: texelMux = t00;
                2'd1: texelMux = t01;
                2'd2: texelMux = t10;
                2'd3: texelMux = t11;
            endcase
        end
    endfunction

    function isPixelOutside;
        input [31 : 0] texCoord;
        input [ 0 : 0] mode;
        begin
            if (mode == CLAMP_TO_EDGE)
            begin
                if (texCoord[31]) // Check if it lower than 0 by only checking the sign bit
                begin
                    isPixelOutside = 1;
                end
                else if ((texCoord >> 15) != 0) // Check if it is greater than one by checking if the integer part is unequal to zero
                begin
                    isPixelOutside = 1;
                end 
                else
                begin
                    isPixelOutside = 0;
                end
            end
            else
            begin
                isPixelOutside = 0;
            end
        end
    endfunction

    // Convert the configured texture size into a 0.5.
    // The texture size comes as a integer value which is intepreted as a normalized 1.0.
    // Assuming a texture size of 4px (16'h100). To normalize this, the point (imaginary) must be moved to the diget bevore the 1.
    // But a 1.0 is different on a 4px texture and on a texture on another size like 16px (16'h10000). Therefore also the 
    // representation of 0.5 changes from 16'h010 to 16'h01000. This is what this function is intended for, to get a normalized
    // 0.5 depending on the size of the texture.
    function [31 : 0] convertToZeroPointFive;
        input [7 : 0] texSize;
        convertToZeroPointFive = 
        {
            1'h0, // sign
            16'h0, // integer part
            texSize[0],
            texSize[1],
            texSize[2],
            texSize[3],
            texSize[4],
            texSize[5],
            texSize[6],
            texSize[7],
            7'h0
        };
    endfunction

    // See convertToZeroPointFive. It is basically the same just for 1.0
    function [31 : 0] convertToOnePointZero;
        input [7 : 0] texSize;
        convertToOnePointZero = 
        {
            1'h0, // sign
            15'h0, // integer part
            texSize[0], // integer part
            texSize[1], // fraction part
            texSize[2],
            texSize[3],
            texSize[4],
            texSize[5],
            texSize[6],
            texSize[7],
            8'h0
        };
    endfunction

    //////////////////////////////////////////////
    // STEP 0
    // Calculate the offset of the texture address based on the LOD
    // Clocks: 1
    //////////////////////////////////////////////
    localparam TEX_SIZE = 8;
    localparam TEX_SIZE_ST = TEX_SIZE * 2;
    localparam TEX_MASK_SIZE = TEX_SIZE + 1;
    localparam TEX_MASK_SIZE_ST = TEX_SIZE_ST + 2;
    localparam TEX_MASK_ONE = { { TEX_SIZE { 1'b0 } }, 1'b1 };
    localparam TEX_MASK_ST_ONE = { { TEX_SIZE_ST { 1'b0 } }, 2'b1 };
    reg [TEX_MASK_SIZE_ST - 1 : 0]  step0_offset;
    reg [ 3 : 0]                    step0_width;
    reg [ 3 : 0]                    step0_height;
    reg [31 : 0]                    step0_texelS; // S16.15
    reg [31 : 0]                    step0_texelT; // S16.15
    reg                             step0_clampS;
    reg                             step0_clampT;

    always @(posedge aclk)
    begin : LodOffsetCalc
        reg [ 3 : 0]                    width;
        reg [ 3 : 0]                    height;
        
        reg [TEX_MASK_SIZE - 1 : 0]     wMask;
        reg [TEX_MASK_SIZE - 1 : 0]     hMask;
        reg [TEX_MASK_SIZE_ST - 1 : 0]  lodMask;
        reg [TEX_MASK_SIZE_ST - 1 : 0]  mask;
        reg [TEX_MASK_SIZE_ST - 1 : 0]  maskFin;
        integer i;

        width = (textureLod < textureSizeWidth) ? textureSizeWidth - textureLod : 0;
        height = (textureLod < textureSizeHeight) ? textureSizeHeight - textureLod : 0;
        lodMask = ~((TEX_MASK_ST_ONE << ((width + height + 1))) - TEX_MASK_ST_ONE);

        wMask = ((TEX_MASK_ONE << textureSizeWidth) - TEX_MASK_ONE);
        hMask = ((TEX_MASK_ONE << textureSizeHeight) - TEX_MASK_ONE);
        for (i = 0; i < TEX_MASK_SIZE; i = i + 1)
        begin
            mask[(TEX_MASK_SIZE_ST - 1) - ((i * 2) + 1)] = wMask[i] ^ hMask[i];
            mask[(TEX_MASK_SIZE_ST - 1) - ((i * 2) + 0)] = wMask[i] | hMask[i];
        end
        maskFin = mask >> (TEX_MASK_SIZE_ST - (textureSizeWidth + textureSizeHeight + 1));
        
        step0_offset <= maskFin & lodMask;
        step0_width <= width;
        step0_height <= height;
        step0_texelS <= texelS;
        step0_texelT <= texelT;
        step0_clampS <= clampS;
        step0_clampT <= clampT;
    end

    //////////////////////////////////////////////
    // STEP 1
    // Build RAM adresses
    // Clocks: 1
    //////////////////////////////////////////////
    reg             step1_texelU0Valid;
    reg             step1_texelU1Valid;
    reg             step1_texelV0Valid;
    reg             step1_texelV1Valid;
    reg  [15 : 0]   step1_subCoordU; // Q0.16
    reg  [15 : 0]   step1_subCoordV; // Q0.16
    
    always @(posedge aclk)
    begin : TexAddrCalc
        reg [31 : 0]                    texelS0; // S16.15
        reg [31 : 0]                    texelS1; // S16.15
        reg [31 : 0]                    texelT0; // S16.15
        reg [31 : 0]                    texelT1; // S16.15

        if (enableHalfPixelOffset)
        begin
            texelS0 = step0_texelS - convertToZeroPointFive(1 << step0_width);
            texelS1 = step0_texelS + convertToZeroPointFive(1 << step0_width);
            texelT0 = step0_texelT - convertToZeroPointFive(1 << step0_height);
            texelT1 = step0_texelT + convertToZeroPointFive(1 << step0_height);
        end
        else
        begin
            texelS0 = step0_texelS;
            texelS1 = step0_texelS + convertToOnePointZero(1 << step0_width);
            texelT0 = step0_texelT;
            texelT1 = step0_texelT + convertToOnePointZero(1 << step0_height);
        end

        step1_texelU0Valid <= !isPixelOutside(texelS0, step0_clampS);
        step1_texelU1Valid <= !isPixelOutside(texelS1, step0_clampS);
        step1_texelV0Valid <= !isPixelOutside(texelT0, step0_clampT);
        step1_texelV1Valid <= !isPixelOutside(texelT1, step0_clampT);

        texelAddr00 <= step0_offset[0 +: 17] + (({ 9'h0, texelT0[7 +: 8] >> (8 - step0_height) } << step0_width) | { 9'h0, texelS0[7 +: 8] >> (8 - step0_width) });
        texelAddr01 <= step0_offset[0 +: 17] + (({ 9'h0, texelT0[7 +: 8] >> (8 - step0_height) } << step0_width) | { 9'h0, texelS1[7 +: 8] >> (8 - step0_width) });
        texelAddr10 <= step0_offset[0 +: 17] + (({ 9'h0, texelT1[7 +: 8] >> (8 - step0_height) } << step0_width) | { 9'h0, texelS0[7 +: 8] >> (8 - step0_width) });
        texelAddr11 <= step0_offset[0 +: 17] + (({ 9'h0, texelT1[7 +: 8] >> (8 - step0_height) } << step0_width) | { 9'h0, texelS1[7 +: 8] >> (8 - step0_width) });
        step1_subCoordU <= { texelS0[0 +: 15], 1'b0 } << step0_width;
        step1_subCoordV <= { texelT0[0 +: 15], 1'b0 } << step0_height;
    end

    //////////////////////////////////////////////
    // STEP 2
    // Wait for data
    // Clocks: 1
    //////////////////////////////////////////////
    wire            step2_texelU0Valid;
    wire            step2_texelU1Valid;
    wire            step2_texelV0Valid;
    wire            step2_texelV1Valid;
    wire [15 : 0]   step2_subCoordU; // Q0.16
    wire [15 : 0]   step2_subCoordV; // Q0.16

    ValueDelay #( .VALUE_SIZE(16), .DELAY(MEMORY_DELAY)) 
        step2_subCoordUDelay ( .clk(aclk), .in(step1_subCoordU), .out(step2_subCoordU));

    ValueDelay #( .VALUE_SIZE(16), .DELAY(MEMORY_DELAY)) 
        step2_subCoordVDelay ( .clk(aclk), .in(step1_subCoordV), .out(step2_subCoordV));

    ValueDelay #( .VALUE_SIZE(1), .DELAY(MEMORY_DELAY)) 
        step2_texelU0ValidDelay ( .clk(aclk), .in(step1_texelU0Valid), .out(step2_texelU0Valid));

    ValueDelay #( .VALUE_SIZE(1), .DELAY(MEMORY_DELAY)) 
        step2_texelU1ValidDelay ( .clk(aclk), .in(step1_texelU1Valid), .out(step2_texelU1Valid));

    ValueDelay #( .VALUE_SIZE(1), .DELAY(MEMORY_DELAY)) 
        step2_texelV0ValidDelay ( .clk(aclk), .in(step1_texelV0Valid), .out(step2_texelV0Valid));

    ValueDelay #( .VALUE_SIZE(1), .DELAY(MEMORY_DELAY)) 
        step2_texelV1ValidDelay ( .clk(aclk), .in(step1_texelV1Valid), .out(step2_texelV1Valid));

    //////////////////////////////////////////////
    // STEP 3
    // Clamp texel quad
    // Clocks: 1
    //////////////////////////////////////////////
    reg  [15 : 0]               step3_subCoordU; // Q0.16
    reg  [15 : 0]               step3_subCoordV; // Q0.16
    reg  [PIXEL_WIDTH - 1 : 0]  step3_texel00;
    reg  [PIXEL_WIDTH - 1 : 0]  step3_texel01; 
    reg  [PIXEL_WIDTH - 1 : 0]  step3_texel10; 
    reg  [PIXEL_WIDTH - 1 : 0]  step3_texel11; 

    always @(posedge aclk)
    begin : ClampTexelQuad
        reg         t00;
        reg         t01;
        reg         t10;
        reg         t11;
        reg [1 : 0] ti00;
        reg [1 : 0] ti01;
        reg [1 : 0] ti10;
        reg [1 : 0] ti11;

        t00 = step2_texelU0Valid & step2_texelV0Valid;
        t01 = step2_texelU1Valid & step2_texelV0Valid;
        t10 = step2_texelU0Valid & step2_texelV1Valid;
        t11 = step2_texelU1Valid & step2_texelV1Valid;
    
        casez ({t11, t10, t01, t00})
            // Imagine a pixel quad as
            // 0 1 
            // 2 3
            // Lets observe 0. Now the following cases can exist:
            // 0 ? : in all cases where 0 is valid -> use 0
            // ? ? 
            4'b???1: ti00 = 0; // Pixel of the quad is in bounds
            // shift out in x direction
            // o 1 : 0 is outside, clamp s requires to use the right side -> use 1
            // o 3 
            4'b1010: ti00 = 1; // Quad is shiftet out in x direction
            // shift out in y direction
            // o o : 0 is outside, clamp t requires to use the bottom -> use 2
            // 2 3 
            4'b1100: ti00 = 2; // Quad is shiftet out in y direction
            // shift out on top left corner
            // o o : 0 is outside only 3 is inside -> use 3
            // o 3 
            4'b1000: ti00 = 3; // Quad is on the corner
            // shift out on top right corner
            // o o : 0 is outside only 2 is inside -> use 2
            // 2 o 
            4'b0100: ti00 = 2; // Quad is on the corner
            // shift out on bottom left corner
            // o 1 : 0 is outside only 1 is inside -> use 1
            // o o 
            4'b0010: ti00 = 1; // Quad is on the corner
            // There should never be a case where this default is required.
            // There should be always a valid texel in the quad and they should always clamp on multiple of 90°
            default: ti00 = 0; // Default all invalid cases
        endcase

        casez ({t11, t10, t01, t00})
            4'b??1?: ti01 = 1; // Pixel of the quad is in bounds
            4'b0101: ti01 = 0; // Quad is shiftet out in x direction
            4'b1100: ti01 = 3; // Quad is shiftet out in y direction
            4'b1000: ti01 = 3; // Quad is on the corner
            4'b0100: ti01 = 2; // Quad is on the corner
            4'b0001: ti01 = 0; // Quad is on the corner
            default: ti01 = 1; // Default all invalid cases
        endcase

        casez ({t11, t10, t01, t00})
            4'b?1??: ti10 = 2; // Pixel of the quad is in bounds
            4'b1010: ti10 = 3; // Quad is shiftet out in x direction
            4'b0011: ti10 = 0; // Quad is shiftet out in y direction
            4'b1000: ti10 = 3; // Quad is on the corner
            4'b0010: ti10 = 1; // Quad is on the corner
            4'b0001: ti10 = 0; // Quad is on the corner
            default: ti10 = 2; // Default all invalid cases
        endcase

        casez ({t11, t10, t01, t00})
            4'b1???: ti11 = 3; // Pixel of the quad is in bounds
            4'b0101: ti11 = 2; // Quad is shiftet out in x direction
            4'b0011: ti11 = 1; // Quad is shiftet out in y direction
            4'b0100: ti11 = 2; // Quad is on the corner
            4'b0010: ti11 = 1; // Quad is on the corner
            4'b0001: ti11 = 0; // Quad is on the corner
            default: ti11 = 3; // Default all invalid cases
        endcase

        // Clamp texel quad
        step3_texel00 <= texelMux(ti00, texelInput00, texelInput01, texelInput10, texelInput11);
        step3_texel01 <= texelMux(ti01, texelInput00, texelInput01, texelInput10, texelInput11);
        step3_texel10 <= texelMux(ti10, texelInput00, texelInput01, texelInput10, texelInput11);
        step3_texel11 <= texelMux(ti11, texelInput00, texelInput01, texelInput10, texelInput11);

        step3_subCoordU <= step2_subCoordU;
        step3_subCoordV <= step2_subCoordV;
    end

    //////////////////////////////////////////////
    // STEP 3
    // Output
    // Clocks: 0
    //////////////////////////////////////////////
    assign texel00 = step3_texel00;
    assign texel01 = step3_texel01;
    assign texel10 = step3_texel10;
    assign texel11 = step3_texel11;

    assign texelSubCoordS = step3_subCoordU;
    assign texelSubCoordT = step3_subCoordV;

endmodule 