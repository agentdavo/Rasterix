package rasterix

import spinal.core._
import spinal.lib._
import spinal.lib.bus.amba4.axi._




case class RasterixEFConfig(
  pixelWidth: Int = 16,
  stencilWidth: Int = 4,
  depthWidth: Int = 16,
  enableStencilBuffer: Boolean = true,
  tmuCount: Int = 2,
  enableMipmapping: Boolean = true,
  cmdStreamWidth: Int = 32,
  fbMemDataWidth: Int = 32,
  textureBufferSize: Int = 17,
  addrWidth: Int = 32,
  idWidth: Int = 8,
  strbWidth: Int = 4,
  fbMemStrbWidth: Int = 4,
  rasterizerFloatPrecision: Int = 26,
  rasterizerFixpointPrecision: Int = 26,
  rasterizerEnableFloatInterpolation: Boolean = false
)




object Rasterix{
  def apply(config: RasterixEFConfig): Rasterix = new Rasterix(config)
}




class Rasterix(config: RasterixEFConfig) extends Component{

  val io = new Bundle{
  }

  // val rasterix = new RasterixEF(config)

}




class RasterixEF(config: RasterixEFConfig) extends BlackBox {

  addGeneric("PIXEL_WIDTH", config.pixelWidth)
  addGeneric("STENCIL_WIDTH", config.stencilWidth)
  addGeneric("DEPTH_WIDTH", config.depthWidth)
  addGeneric("ENABLE_STENCIL_BUFFER", config.enableStencilBuffer)
  addGeneric("TMU_COUNT", config.tmuCount)
  addGeneric("ENABLE_MIPMAPPING", config.enableMipmapping)
  addGeneric("CMD_STREAM_WIDTH", config.cmdStreamWidth)
  addGeneric("FB_MEM_DATA_WIDTH", config.fbMemDataWidth)
  addGeneric("TEXTURE_BUFFER_SIZE", config.textureBufferSize)
  addGeneric("ADDR_WIDTH", config.addrWidth)
  addGeneric("ID_WIDTH", config.idWidth)
  addGeneric("STRB_WIDTH", config.strbWidth)
  addGeneric("FB_MEM_STRB_WIDTH", config.fbMemStrbWidth)
  addGeneric("RASTERIZER_FLOAT_PRECISION", config.rasterizerFloatPrecision)
  addGeneric("RASTERIZER_FIXPOINT_PRECISION", config.rasterizerFixpointPrecision)
  addGeneric("RASTERIZER_ENABLE_FLOAT_INTERPOLATION", config.rasterizerEnableFloatInterpolation)

  val io = new Bundle {

    val aclk = in Bool()
    val resetn = in Bool()
    
    // AXI Stream command interface
    val s_cmd_axis_tvalid = in Bool()
    val s_cmd_axis_tready = out Bool()
    val s_cmd_axis_tlast = in Bool()
    val s_cmd_axis_tdata = in Bits(config.cmdStreamWidth bits)
    
    // Framebuffer output
    val m_framebuffer_axis_tvalid = out Bool()
    val m_framebuffer_axis_tready = in Bool()
    val m_framebuffer_axis_tlast = out Bool()
    val m_framebuffer_axis_tdata = out Bits(config.fbMemDataWidth bits)
    val swap_fb = out Bool()
    val fb_addr = out UInt(config.addrWidth bits)
    val fb_swapped = in Bool()
    
    // Common memory interface
    val m_common_axi_awid = out UInt(config.idWidth bits)
    val m_common_axi_awaddr = out UInt(config.addrWidth bits)
    val m_common_axi_awlen = out UInt(8 bits)
    val m_common_axi_awsize = out UInt(3 bits)
    val m_common_axi_awburst = out UInt(2 bits)
    val m_common_axi_awlock = out Bool()
    val m_common_axi_awcache = out UInt(4 bits)
    val m_common_axi_awprot = out UInt(3 bits)
    val m_common_axi_awvalid = out Bool()
    val m_common_axi_awready = in Bool()
    val m_common_axi_wdata = out Bits(config.cmdStreamWidth bits)
    val m_common_axi_wstrb = out Bits(config.strbWidth bits)
    val m_common_axi_wlast = out Bool()
    val m_common_axi_wvalid = out Bool()
    val m_common_axi_wready = in Bool()
    val m_common_axi_bid = in UInt(config.idWidth bits)
    val m_common_axi_bresp = in UInt(2 bits)
    val m_common_axi_bvalid = in Bool()
    val m_common_axi_bready = out Bool()
    val m_common_axi_arid = out UInt(config.idWidth bits)
    val m_common_axi_araddr = out UInt(config.addrWidth bits)
    val m_common_axi_arlen = out UInt(8 bits)
    val m_common_axi_arsize = out UInt(3 bits)
    val m_common_axi_arburst = out UInt(2 bits)
    val m_common_axi_arlock = out Bool()
    val m_common_axi_arcache = out UInt(4 bits)
    val m_common_axi_arprot = out UInt(3 bits)
    val m_common_axi_arvalid = out Bool()
    val m_common_axi_arready = in Bool()
    val m_common_axi_rid = in UInt(config.idWidth bits)
    val m_common_axi_rdata = in Bits(config.cmdStreamWidth bits)
    val m_common_axi_rresp = in UInt(2 bits)
    val m_common_axi_rlast = in Bool()
    val m_common_axi_rvalid = in Bool()
    val m_common_axi_rready = out Bool()

    // Color Buffer AXI Interface
    val m_color_axi_awid = out UInt(config.idWidth bits)
    val m_color_axi_awaddr = out UInt(config.addrWidth bits)
    val m_color_axi_awlen = out UInt(8 bits)
    val m_color_axi_awsize = out UInt(3 bits)
    val m_color_axi_awburst = out UInt(2 bits)
    val m_color_axi_awlock = out Bool()
    val m_color_axi_awcache = out UInt(4 bits)
    val m_color_axi_awprot = out UInt(3 bits)
    val m_color_axi_awvalid = out Bool()
    val m_color_axi_awready = in Bool()
    val m_color_axi_wdata = out Bits(config.fbMemDataWidth bits)
    val m_color_axi_wstrb = out Bits(config.fbMemDataWidth bits)
    val m_color_axi_wlast = out Bool()
    val m_color_axi_wvalid = out Bool()
    val m_color_axi_wready = in Bool()
    val m_color_axi_bid = in UInt(config.idWidth bits)
    val m_color_axi_bresp = in UInt(2 bits)
    val m_color_axi_bvalid = in Bool()
    val m_color_axi_bready = out Bool()
    val m_color_axi_arid = out UInt(config.idWidth bits)
    val m_color_axi_araddr = out UInt(config.addrWidth bits)
    val m_color_axi_arlen = out UInt(8 bits)
    val m_color_axi_arsize = out UInt(3 bits)
    val m_color_axi_arburst = out UInt(2 bits)
    val m_color_axi_arlock = out Bool()
    val m_color_axi_arcache = out UInt(4 bits)
    val m_color_axi_arprot = out UInt(3 bits)
    val m_color_axi_arvalid = out Bool()
    val m_color_axi_arready = in Bool()
    val m_color_axi_rid = in UInt(config.idWidth bits)
    val m_color_axi_rdata = in Bits(config.fbMemDataWidth bits)
    val m_color_axi_rresp = in UInt(2 bits)
    val m_color_axi_rlast = in Bool()
    val m_color_axi_rvalid = in Bool()
    val m_color_axi_rready = out Bool()

   // Depth Buffer AXI Interface
   val m_depth_axi_awid = out UInt(config.idWidth bits)
   val m_depth_axi_awaddr = out UInt(config.addrWidth bits)
   val m_depth_axi_awlen = out UInt(8 bits)
   val m_depth_axi_awsize = out UInt(3 bits)
   val m_depth_axi_awburst = out UInt(2 bits)
   val m_depth_axi_awlock = out Bool()
   val m_depth_axi_awcache = out UInt(4 bits)
   val m_depth_axi_awprot = out UInt(3 bits)
   val m_depth_axi_awvalid = out Bool()
   val m_depth_axi_awready = in Bool()
   val m_depth_axi_wdata = out Bits(config.fbMemDataWidth bits)
   val m_depth_axi_wstrb = out Bits(config.fbMemDataWidth bits)
   val m_depth_axi_wlast = out Bool()
   val m_depth_axi_wvalid = out Bool()
   val m_depth_axi_wready = in Bool()
   val m_depth_axi_bid = in UInt(config.idWidth bits)
   val m_depth_axi_bresp = in UInt(2 bits)
   val m_depth_axi_bvalid = in Bool()
   val m_depth_axi_bready = out Bool()
   val m_depth_axi_arid = out UInt(config.idWidth bits)
   val m_depth_axi_araddr = out UInt(config.addrWidth bits)
   val m_depth_axi_arlen = out UInt(8 bits)
   val m_depth_axi_arsize = out UInt(3 bits)
   val m_depth_axi_arburst = out UInt(2 bits)
   val m_depth_axi_arlock = out Bool()
   val m_depth_axi_arcache = out UInt(4 bits)
   val m_depth_axi_arprot = out UInt(3 bits)
   val m_depth_axi_arvalid = out Bool()
   val m_depth_axi_arready = in Bool()
   val m_depth_axi_rid = in UInt(config.idWidth bits)
   val m_depth_axi_rdata = in Bits(config.fbMemDataWidth bits)
   val m_depth_axi_rresp = in UInt(2 bits)
   val m_depth_axi_rlast = in Bool()
   val m_depth_axi_rvalid = in Bool()
   val m_depth_axi_rready = out Bool()


   // Stencil Buffer AXI Interface
   val m_stencil_axi_awid = out UInt(config.idWidth bits)
   val m_stencil_axi_awaddr = out UInt(config.addrWidth bits)
   val m_stencil_axi_awlen = out UInt(8 bits)
   val m_stencil_axi_awsize = out UInt(3 bits)
   val m_stencil_axi_awburst = out UInt(2 bits)
   val m_stencil_axi_awlock = out Bool()
   val m_stencil_axi_awcache = out UInt(4 bits)
   val m_stencil_axi_awprot = out UInt(3 bits)
   val m_stencil_axi_awvalid = out Bool()
   val m_stencil_axi_awready = in Bool()
   val m_stencil_axi_wdata = out Bits(config.fbMemDataWidth bits)
   val m_stencil_axi_wstrb = out Bits(config.fbMemDataWidth bits)
   val m_stencil_axi_wlast = out Bool()
   val m_stencil_axi_wvalid = out Bool()
   val m_stencil_axi_wready = in Bool()
   val m_stencil_axi_bid = in UInt(config.idWidth bits)
   val m_stencil_axi_bresp = in UInt(2 bits)
   val m_stencil_axi_bvalid = in Bool()
   val m_stencil_axi_bready = out Bool()
   val m_stencil_axi_arid = out UInt(config.idWidth bits)
   val m_stencil_axi_araddr = out UInt(config.addrWidth bits)
   val m_stencil_axi_arlen = out UInt(8 bits)
   val m_stencil_axi_arsize = out UInt(3 bits)
   val m_stencil_axi_arburst = out UInt(2 bits)
   val m_stencil_axi_arlock = out Bool()
   val m_stencil_axi_arcache = out UInt(4 bits)
   val m_stencil_axi_arprot = out UInt(3 bits)
   val m_stencil_axi_arvalid = out Bool()
   val m_stencil_axi_arready = in Bool()
   val m_stencil_axi_rid = in UInt(config.idWidth bits)
   val m_stencil_axi_rdata = in Bits(config.fbMemDataWidth bits)
   val m_stencil_axi_rresp = in UInt(2 bits)
   val m_stencil_axi_rlast = in Bool()
   val m_stencil_axi_rvalid = in Bool()
   val m_stencil_axi_rready = out Bool()
  
  }

  addRTLPath("./rtl/Rasterix/AttributeInterpolator.v")
  addRTLPath("./rtl/Rasterix/AttributeF2XConverter.v")
  addRTLPath("./rtl/Rasterix/AttributeInterpolatorX.v")
  addRTLPath("./rtl/Rasterix/AttributeInterpolationX.v")
  addRTLPath("./rtl/Rasterix/AttributePerspectiveCorrectionX.v")
  addRTLPath("./rtl/Rasterix/ColorBlender.v")
  addRTLPath("./rtl/Rasterix/ColorInterpolator.v")
  addRTLPath("./rtl/Rasterix/ColorMixerSigned.v")
  addRTLPath("./rtl/Rasterix/ColorMixer.v")
  addRTLPath("./rtl/Rasterix/CommandParser.v")
  addRTLPath("./rtl/Rasterix/DmaStreamEngine.v")
  addRTLPath("./rtl/Rasterix/DualPortRam.v")
  addRTLPath("./rtl/Rasterix/Fog.v")
  addRTLPath("./rtl/Rasterix/FrameBuffer.v")
  addRTLPath("./rtl/Rasterix/FramebufferReader.v")
  addRTLPath("./rtl/Rasterix/FramebufferSerializer.v")
  addRTLPath("./rtl/Rasterix/FramebufferWriter.v")
  addRTLPath("./rtl/Rasterix/FramebufferWriterClear.v")
  addRTLPath("./rtl/Rasterix/FramebufferWriterStrobeGen.v")
  addRTLPath("./rtl/Rasterix/FunctionInterpolator.v")
  addRTLPath("./rtl/Rasterix/LodCalculator.v")
  addRTLPath("./rtl/Rasterix/MipmapOptimizedRam.v")
  addRTLPath("./rtl/Rasterix/MemoryReadRequestGenerator.v")
  addRTLPath("./rtl/Rasterix/PerFragmentPipeline.v")
  addRTLPath("./rtl/Rasterix/PixelPipeline.v")
  addRTLPath("./rtl/Rasterix/PixelUtil.v")
  addRTLPath("./rtl/Rasterix/RasterixEF.v")
  addRTLPath("./rtl/Rasterix/RasterixRenderCore.v")
  addRTLPath("./rtl/Rasterix/Rasterizer.v")
  addRTLPath("./rtl/Rasterix/RasterizerCommands.v")
  addRTLPath("./rtl/Rasterix/RegisterAndDescriptorDefines.v")
  addRTLPath("./rtl/Rasterix/RegisterBank.v")
  addRTLPath("./rtl/Rasterix/StencilOp.v")
  addRTLPath("./rtl/Rasterix/StreamBarrier.v")
  addRTLPath("./rtl/Rasterix/StreamConcatFifo.v")
  addRTLPath("./rtl/Rasterix/StreamFramebuffer.v")
  addRTLPath("./rtl/Rasterix/StreamSemaphore.v")
  addRTLPath("./rtl/Rasterix/TestFunc.v")
  addRTLPath("./rtl/Rasterix/TexEnv.v")
  addRTLPath("./rtl/Rasterix/TextureBuffer.v")
  addRTLPath("./rtl/Rasterix/TextureFilter.v")
  addRTLPath("./rtl/Rasterix/TextureMappingUnit.v")
  addRTLPath("./rtl/Rasterix/TextureSampler.v")
  addRTLPath("./rtl/Rasterix/TrueDualPortRam.v")
  addRTLPath("./rtl/Rasterix/StencilOp.v")
  addRTLPath("./rtl/Float/rtl/float/XRecip.v")
  addRTLPath("./rtl/Float/rtl/float/ComputeRecip.v")
  addRTLPath("./rtl/Float/rtl/float/FindExponent.v")
  addRTLPath("./rtl/Float/rtl/float/FloatAdd.v")
  addRTLPath("./rtl/Float/rtl/float/FloatRecip.v")
  addRTLPath("./rtl/Float/rtl/float/FloatFastRecip.v")
  addRTLPath("./rtl/Float/rtl/float/FloatMul.v")
  addRTLPath("./rtl/Float/rtl/float/FloatSub.v")
  addRTLPath("./rtl/Float/rtl/float/FloatToInt.v")
  addRTLPath("./rtl/Float/rtl/float/IntToFloat.v")
  addRTLPath("./rtl/Float/rtl/float/ValueDelay.v")
  addRTLPath("./rtl/Float/rtl/float/ValueTrack.v")

  mapCurrentClockDomain(io.aclk, io.resetn)
  noIoPrefix()

}