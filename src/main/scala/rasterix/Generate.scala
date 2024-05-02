package rasterix

import spinal.core._
import spinal.lib.LatencyAnalysis
import spinal.lib.bus.misc.SizeMapping
import spinal.lib.bus.tilelink.{M2sTransfers, SizeRange}

import scala.collection.mutable.ArrayBuffer

object Generate extends App {

  val config = SpinalConfig(targetDirectory = "hw/gen")

  config.generateVerilog(Rasterix(RasterixEFConfig()))

}