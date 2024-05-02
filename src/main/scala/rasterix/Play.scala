package rasterix

import spinal.core._
import spinal.lib._
import spinal.lib.bus.amba4.axi._




case class RasterixEFConfig(
  pixelWidth          : Int = 16,
  stencilWidth        : Int = 4,
  depthWidth          : Int = 16,
  enableStencilBuffer : Boolean = true,
  tmuCount            : Int = 2,
  enableMipmapping    : Boolean = true,
  cmdStreamWidth      : Int = 32,
  fbMemDataWidth      : Int = 32,
  textureBufferSize   : Int = 17,
  addrWidth           : Int = 32,
  idWidth             : Int = 8,
  strbWidth           : Int = 4,
  fbMemStrbWidth      : Int = 4,
  rasterizerFloatPrecision           : Int = 26,
  rasterizerFixpointPrecision        : Int = 26,
  rasterizerEnableFloatInterpolation : Boolean = false
)




class RasterixEF(config: RasterixEFConfig) extends Component {
  import config._

  val io = new Bundle {

    // Clock and reset
    val aclk = in Bool
    val resetn = in Bool

    // AXI Stream command interface
    val cmd             = slave(Stream(Bits(cmdStreamWidth bits)))
    val frameBuf        = master(Stream(Bits(fbMemDataWidth bits)))
    val swapFrameBuf    = out Bool()
    val fbAddress       = out Bits(addrWidth bits)
    val frameBufSwapped = in Bool()
    
  }

  val dmaEngine     = new DmaStreamEngine(CMD_STREAM_WIDTH = 64, ADDR_WIDTH = 28, ID_WIDTH = 8)
  val colorBuffer   = new StreamFramebuffer(config)
  val depthBuffer   = new StreamFramebuffer(config)
  val stencilBuffer = new StreamFramebuffer(config)
  val renderCore    = new RasterixRenderCore(config)

  // connect clocks
  dmaEngine.io.aclk       := io.aclk
  dmaEngine.io.resetn     := io.resetn
  colorBuffer.io.aclk     := io.aclk
  colorBuffer.io.resetn   := io.resetn
  depthBuffer.io.aclk     := io.aclk
  depthBuffer.io.resetn   := io.resetn
  stencilBuffer.io.aclk   := io.aclk
  stencilBuffer.io.resetn := io.resetn
  renderCore.io.aclk      := io.aclk
  renderCore.io.resetn    := io.resetn
  
}




class DmaStreamEngine(STREAM_WIDTH     : Int = 32,
                      ADDR_WIDTH       : Int = 16,
                      STRB_WIDTH       : Int = (STREAM_WIDTH / 8),
                      ID_WIDTH         : Int = 8,
                      AUTO_CH_IN       : Int = 0,
                      AUTO_CH_OUT      : Int = 0,
                      AUTO_DATA_SIZE   : Int = 0,
                      AUTO_ADDRESS     : Int = 32 ) extends BlackBox {

  addGeneric("STREAM_WIDTH", STREAM_WIDTH)
  addGeneric("ADDR_WIDTH", ADDR_WIDTH)
  addGeneric("STRB_WIDTH", STRB_WIDTH)
  addGeneric("AUTO_CH_IN", AUTO_CH_IN)
  addGeneric("AUTO_CH_OUT", AUTO_CH_OUT)
  addGeneric("AUTO_DATA_SIZE", AUTO_DATA_SIZE)
  addGeneric("AUTO_ADDRESS", AUTO_ADDRESS)
  
  val io = new Bundle {
    
  // Clock and reset
  val aclk = in Bool
  val resetn = in Bool

  // AXI Stream interfaces for commands and framebuffers
  val m_st1_axis_tvalid = out Bool
  val m_st1_axis_tready = in Bool
  val m_st1_axis_tlast = out Bool
  val m_st1_axis_tdata = out Bits(STREAM_WIDTH bits)

  val s_st1_axis_tvalid = in Bool
  val s_st1_axis_tready = out Bool
  val s_st1_axis_tlast = in Bool
  val s_st1_axis_tdata = in Bits(STREAM_WIDTH bits)

  val m_st0_axis_tvalid = out Bool
  val m_st0_axis_tready = in Bool
  val m_st0_axis_tlast = out Bool
  val m_st0_axis_tdata = out Bits(STREAM_WIDTH bits)

  val s_st0_axis_tvalid = in Bool
  val s_st0_axis_tready = out Bool
  val s_st0_axis_tlast = in Bool
  val s_st0_axis_tdata = in Bits(STREAM_WIDTH bits)

  // AXI Memory Interface
  val m_mem_axi_awid = out Bits(ID_WIDTH bits)
  val m_mem_axi_awaddr = out UInt(ADDR_WIDTH bits)
  val m_mem_axi_awlen = out UInt(8 bits)
  val m_mem_axi_awsize = out UInt(3 bits)
  val m_mem_axi_awburst = out Bits(2 bits)
  val m_mem_axi_awlock = out Bits(1 bits)
  val m_mem_axi_awcache = out Bits(4 bits)
  val m_mem_axi_awprot = out Bits(3 bits)
  val m_mem_axi_awvalid = out Bool
  val m_mem_axi_awready = in Bool
  val m_mem_axi_wdata = out Bits(STREAM_WIDTH bits)
  val m_mem_axi_wstrb = out Bits((STREAM_WIDTH/8) bits)
  val m_mem_axi_wlast = out Bool
  val m_mem_axi_wvalid = out Bool
  val m_mem_axi_wready = in Bool
  val m_mem_axi_bid = in Bits(ID_WIDTH bits)
  val m_mem_axi_bresp = in Bits(2 bits)
  val m_mem_axi_bvalid = in Bool
  val m_mem_axi_bready = out Bool
  val m_mem_axi_arid = out Bits(ID_WIDTH bits)
  val m_mem_axi_araddr = out UInt(ADDR_WIDTH bits)
  val m_mem_axi_arlen = out UInt(8 bits)
  val m_mem_axi_arsize = out UInt(3 bits)
  val m_mem_axi_arburst = out Bits(2 bits)
  val m_mem_axi_arlock = out Bits(1 bits)
  val m_mem_axi_arcache = out Bits(4 bits)
  val m_mem_axi_arprot = out Bits(3 bits)
  val m_mem_axi_arvalid = out Bool
  val m_mem_axi_arready = in Bool
  val m_mem_axi_rid = in Bits(ID_WIDTH bits)
  val m_mem_axi_rdata = in Bits(STREAM_WIDTH bits)
  val m_mem_axi_rresp = in Bits(2 bits)
  val m_mem_axi_rlast = in Bool
  val m_mem_axi_rvalid = in Bool
  val m_mem_axi_rready = out Bool
  }

  addRTLPath("./rtl/Rasterix/DmaStreamEngine.v")
  mapCurrentClockDomain(io.aclk, io.resetn)
  noIoPrefix()
}




class StreamFramebuffer(DATA_WIDTH : Int = 32,
                        ADDR_WIDTH : Int = 32,
                        STRB_WIDTH : Int = (DATA_WIDTH / 8),
                        ID_WIDTH   : Int = 8,
                        X_BIT_WIDTH: Int = 11,
                        Y_BIT_WIDTH: Int = 11,
                        PIXEL_WIDTH: Int = 16) extends BlackBox {

  val PIXEL_MASK_WIDTH = PIXEL_WIDTH / 8
  val PIXEL_WIDTH_LG = log2Up(PIXEL_MASK_WIDTH)

  addGeneric("DATA_WIDTH", DATA_WIDTH)
  addGeneric("ADDR_WIDTH", ADDR_WIDTH)
  addGeneric("STRB_WIDTH", STRB_WIDTH)
  addGeneric("ID_WIDTH", ID_WIDTH)
  addGeneric("X_BIT_WIDTH", X_BIT_WIDTH)
  addGeneric("Y_BIT_WIDTH", Y_BIT_WIDTH)
  addGeneric("PIXEL_WIDTH", PIXEL_WIDTH)

  val io = new Bundle {
    
    val aclk = in Bool
    val resetn = in Bool

    // Configs
    val confAddr = in UInt(ADDR_WIDTH bits)
    val confEnableScissor = in Bool
    val confScissorStartX = in UInt(X_BIT_WIDTH bits)
    val confScissorStartY = in UInt(Y_BIT_WIDTH bits)
    val confScissorEndX = in UInt(X_BIT_WIDTH bits)
    val confScissorEndY = in UInt(Y_BIT_WIDTH bits)
    val confXResolution = in UInt(X_BIT_WIDTH bits)
    val confYResolution = in UInt(Y_BIT_WIDTH bits)
    val confMask = in Bits(PIXEL_MASK_WIDTH bits)
    val confClearColor = in Bits(PIXEL_WIDTH bits)

    // Cmd interface
    val apply = in Bool
    val applied = out Bool

    // Fetch interface
    val s_fetch_arvalid = in Bool
    val s_fetch_arlast = in Bool
    val s_fetch_arready = out Bool
    val s_fetch_araddr = in UInt(ADDR_WIDTH bits)

    // Framebuffer read interface
    val s_frag_rvalid = out Bool
    val s_frag_rready = in Bool
    val s_frag_rdata = out Bits(PIXEL_WIDTH bits)
    val s_frag_rlast = out Bool

    // Framebuffer write interface
    val s_frag_wvalid = in Bool
    val s_frag_wlast = in Bool
    val s_frag_wready = out Bool
    val s_frag_wdata = in Bits(PIXEL_WIDTH bits)
    val s_frag_wstrb = in Bool
    val s_frag_waddr = in UInt(ADDR_WIDTH bits)
    val s_frag_wxpos = in UInt(X_BIT_WIDTH bits)
    val s_frag_wypos = in UInt(Y_BIT_WIDTH bits)

    // AXI Memory Interface
    val m_mem_axi_awid = out Bits(ID_WIDTH bits)
    val m_mem_axi_awaddr = out UInt(ADDR_WIDTH bits)
    val m_mem_axi_awlen = out UInt(8 bits)
    val m_mem_axi_awsize = out UInt(3 bits)
    val m_mem_axi_awburst = out Bits(2 bits)
    val m_mem_axi_awlock = out Bits(1 bits)
    val m_mem_axi_awcache = out Bits(4 bits)
    val m_mem_axi_awprot = out Bits(3 bits)
    val m_mem_axi_awvalid = out Bool
    val m_mem_axi_awready = in Bool
    
    val m_mem_axi_wdata = out Bits(DATA_WIDTH bits)
    val m_mem_axi_wstrb = out Bits(STRB_WIDTH bits)
    val m_mem_axi_wlast = out Bool
    val m_mem_axi_wvalid = out Bool
    val m_mem_axi_wready = in Bool
    
    val m_mem_axi_bid = in Bits(ID_WIDTH bits)
    val m_mem_axi_bresp = in Bits(2 bits)
    val m_mem_axi_bvalid = in Bool
    val m_mem_axi_bready = out Bool
    
    val m_mem_axi_arid = out Bits(ID_WIDTH bits)
    val m_mem_axi_araddr = out UInt(ADDR_WIDTH bits)
    val m_mem_axi_arlen = out UInt(8 bits)
    val m_mem_axi_arsize = out UInt(3 bits)
    val m_mem_axi_arburst = out Bits(2 bits)
    val m_mem_axi_arlock = out Bits(1 bits)
    val m_mem_axi_arcache = out Bits(4 bits)
    val m_mem_axi_arprot = out Bits(3 bits)
    val m_mem_axi_arvalid = out Bool
    val m_mem_axi_arready = in Bool
    
    val m_mem_axi_rid = in Bits(ID_WIDTH bits)
    val m_mem_axi_rdata = in Bits(CMD_STREAM_WIDTH bits)
    val m_mem_axi_rresp = in Bits(2 bits)
    val m_mem_axi_rlast = in Bool
    val m_mem_axi_rvalid = in Bool
    val m_mem_axi_rready = out Bool
  }
  
  addRTLPath("./rtl/StreamFramebuffer.v")
  mapCurrentClockDomain(io.aclk, io.resetn)
  noIoPrefix()
} 





class RasterixRenderCore(config: RasterixEFConfig) extends BlackBox {
  import config._

  val io = new Bundle {
    val aclk = in Bool()
    val resetn = in Bool()
  }

  addRTLPath("./rtl/Rasterix/RasterixRenderCore.v")
  mapCurrentClockDomain(io.aclk, io.resetn)
  noIoPrefix()
}
