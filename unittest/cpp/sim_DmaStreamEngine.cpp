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

// #define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
// #include "../Unittests/3rdParty/catch.hpp"

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "3rdParty/catch.hpp"

// Include common routines
#include <verilated.h>

// Include model header, generated from Verilating "top.v"
#include "VDmaStreamEngine.h"

void clk(VDmaStreamEngine& t)
{
    t.aclk = 0;
    t.eval();
    t.aclk = 1;
    t.eval();
}

void reset(VDmaStreamEngine& t)
{
    t.resetn = 0;

    t.m_cmd_axis_tready = 0;

    t.s_cmd_axis_tvalid = 0;
    t.s_cmd_axis_tlast = 0;
    t.s_cmd_axis_tdata = 0;

    clk(t);
    
    t.resetn = 1;
    clk(t);
}

TEST_CASE("Stream data simple", "[Stream]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x4000'0000;
    static constexpr uint32_t SIZE = 0x0000'0008; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 8 bytes from one stream interface to another

    // Tell the command parser that we are ready to receive data
    t.m_cmd_axis_tready = 1;

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 2; i++) 
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Set first 4 bytes of data
        t.s_cmd_axis_tdata = 0xaa00ff55;

        // Process first 4 bytes of data
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xaa00ff55);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Set the next 4 bytes of data
        t.s_cmd_axis_tdata = 0xff00ff00;

        clk(t);

        // STREAM ////////////////////////////
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xff00ff00);
        REQUIRE(t.m_cmd_axis_tlast == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // IDLE /////////////////////////////
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}

TEST_CASE("Stream data interrupted from master", "[Stream]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x4000'0000;
    static constexpr uint32_t SIZE = 0x0000'0008; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 8 bytes from one stream interface to another

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 2; i++)
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        // Set master port to not ready
        t.m_cmd_axis_tready = 0;

        // Process command
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Set first 4 bytes of data
        t.s_cmd_axis_tdata = 0xaa00ff55;

        // Set master port to not ready
        t.m_cmd_axis_tready = 0;

        // Process first 4 bytes of data
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xaa00ff55);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        // Master port is not ready, but we can fetch data
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Set the next 4 bytes of data
        t.s_cmd_axis_tdata = 0xff00ff00;

        // Set master to not ready because we have no data right now
        t.m_cmd_axis_tready = 0;

        clk(t);
            
        // STREAM ////////////////////////////
        // Outputs
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xaa00ff55);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        // Master port is not ready, so we interrupting the stream on the slave
        REQUIRE(t.s_cmd_axis_tready == 0);

        // Set the next 4 bytes of data
        t.s_cmd_axis_tdata = 0xff00ff00;

        // Set master to ready
        t.m_cmd_axis_tready = 1;

        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xff00ff00);
        REQUIRE(t.m_cmd_axis_tlast == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        // Set master port to not ready
        t.m_cmd_axis_tready = 0;

        clk(t);

        // STREAM ////////////////////////////
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xff00ff00);
        REQUIRE(t.m_cmd_axis_tlast == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        // Set master to ready
        t.m_cmd_axis_tready = 1;

        clk(t);

        // IDLE /////////////////////////////

        // Last cycle and waiting for new command
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }


    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}

TEST_CASE("Stream data interrupted from slave", "[Stream]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x4000'0000;
    static constexpr uint32_t SIZE = 0x0000'0008; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 8 bytes from one stream interface to another

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 2; i++)
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        // Set master port will always be ready
        t.m_cmd_axis_tready = 1;

        // Process command
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // No valid data because slave has no data
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        
        // Telling the slave we can receive data
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Set first 4 bytes of data
        t.s_cmd_axis_tdata = 0xaa00ff55;

        // Set slave port to not ready
        t.s_cmd_axis_tvalid = 0;

        // Process first 4 bytes of data
        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // No valid data because slave has no data
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        // Telling the slave we can receive data
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Set the next 4 bytes of data
        t.s_cmd_axis_tdata = 0xaa00ff55;

        // Set slave port to ready
        t.s_cmd_axis_tvalid = 1;

        clk(t);
            
        // STREAM ////////////////////////////
        // Outputs
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xaa00ff55);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        // Telling the slave we can receive data
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Set slave port to not ready
        t.s_cmd_axis_tvalid = 0;

        clk(t);

        // STREAM ////////////////////////////
        // Outputs
        // No valid data because slave has no data
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        // Telling the slave we can receive data
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Set the next 4 bytes of data
        t.s_cmd_axis_tdata = 0xff00ff00;

        // Set slave port to ready
        t.s_cmd_axis_tvalid = 1;

        clk(t);

        // STREAM ////////////////////////////
        // Read first 4 bytes from master interface 
        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 0xff00ff00);
        REQUIRE(t.m_cmd_axis_tlast == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // IDLE /////////////////////////////

        // Last cycle and waiting for new command
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        REQUIRE(t.m_cmd_axis_tlast == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }


    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}

TEST_CASE("Store data simple", "[Memory]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x1000'0000;
    static constexpr uint32_t SIZE = 0x0000'0100; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 256 bytes from one stream interface to another
    static constexpr uint32_t BEATS_PER_TRANSFER = 128 / 4;

    // Tell the command parser that we are ready to receive data
    t.m_cmd_axis_tready = 1;
    t.m_mem_axi_awready = 1;


    t.m_mem_axi_wready = 0;

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 2; i++) 
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // COMMAND ADDR //////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = 0x100;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // First test the address channel (data and address are independent)
        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        t.s_cmd_axis_tvalid = 0;

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 1);
        REQUIRE(t.m_mem_axi_awaddr == 0x100);
        
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 1);
        REQUIRE(t.m_mem_axi_awaddr == 0x180);
        
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 1);


        clk(t);

        // Now check the data channel
        // STORE DATA ////////////////////////
        // Initial value
        REQUIRE(t.s_cmd_axis_tready == 1);

        REQUIRE(t.m_mem_axi_wvalid == 0);

        t.m_mem_axi_wready = 1;

        t.s_cmd_axis_tdata = 0;
        t.s_cmd_axis_tvalid = 1;

        clk(t);

        // STORE DATA ////////////////////////
        // Output contains the data from all elemets except the last
        for (uint32_t i = 1; i < BEATS_PER_TRANSFER; i++)
        {
            REQUIRE(t.s_cmd_axis_tready == 1);

            REQUIRE(t.m_mem_axi_wvalid == 1);
            REQUIRE(t.m_mem_axi_wdata == i - 1);
            REQUIRE(t.m_mem_axi_wlast == 0);

            t.s_cmd_axis_tdata = i;

            clk(t);
        }

        // Second transfer
        // STORE DATA ////////////////////////
        // Check the last element and initialize the next transfer
        REQUIRE(t.s_cmd_axis_tready == 1);

        REQUIRE(t.m_mem_axi_wvalid == 1);
        REQUIRE(t.m_mem_axi_wdata == 31);
        REQUIRE(t.m_mem_axi_wlast == 1);

        t.s_cmd_axis_tdata = 0;

        clk(t);

        // STORE DATA ////////////////////////
        // Output contains the data from all elemets except the last
        for (uint32_t i = 1; i < BEATS_PER_TRANSFER; i++)
        {
            REQUIRE(t.s_cmd_axis_tready == 1);

            REQUIRE(t.m_mem_axi_wvalid == 1);
            REQUIRE(t.m_mem_axi_wdata == i - 1);
            REQUIRE(t.m_mem_axi_wlast == 0);

            t.s_cmd_axis_tdata = i;

            clk(t);
        }

        // Second transfer
        // STORE DATA ////////////////////////
        // Check the last element
        REQUIRE(t.s_cmd_axis_tready == 0);

        REQUIRE(t.m_mem_axi_wvalid == 1);
        REQUIRE(t.m_mem_axi_wdata == 31);
        REQUIRE(t.m_mem_axi_wlast == 1);

        clk(t);

        // IDLE /////////////////////////////
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}

TEST_CASE("Memset data simple", "[Memory]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x3000'0000;
    static constexpr uint32_t SIZE = 0x0000'0100; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 256 bytes from one stream interface to another
    static constexpr uint32_t BEATS_PER_TRANSFER = 128 / 4;

    // Tell the command parser that we are ready to receive data
    t.m_cmd_axis_tready = 1;
    t.m_mem_axi_awready = 1;


    t.m_mem_axi_wready = 0;

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 2; i++) 
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        t.m_mem_axi_wready = 0;

        // Process command
        clk(t);

        // COMMAND ADDR //////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = 0x100;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // COMMAND VAL ///////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = 0xFF0055AA;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // First test the address channel (data and address are independent)
        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        t.s_cmd_axis_tvalid = 0;

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 1);
        REQUIRE(t.m_mem_axi_awaddr == 0x100);
        
        REQUIRE(t.m_mem_axi_wvalid == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 1);
        REQUIRE(t.m_mem_axi_awaddr == 0x180);
        
        REQUIRE(t.m_mem_axi_wvalid == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // STORE ADDR ////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_awvalid == 0);
        
        REQUIRE(t.m_mem_axi_wvalid == 1);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // STORE DATA ////////////////////////
        // Output contains the data from all elemets except the last
        t.m_mem_axi_wready = 1;
        for (uint32_t i = 0; i < BEATS_PER_TRANSFER - 1; i++)
        {
            REQUIRE(t.s_cmd_axis_tready == 0);

            REQUIRE(t.m_mem_axi_wvalid == 1);
            REQUIRE(t.m_mem_axi_wdata == 0xFF0055AA);
            REQUIRE(t.m_mem_axi_wlast == 0);

            clk(t);
        }

        // Second transfer
        // STORE DATA ////////////////////////
        // Check the last element and initialize the next transfer
        REQUIRE(t.s_cmd_axis_tready == 0);

        REQUIRE(t.m_mem_axi_wvalid == 1);
        REQUIRE(t.m_mem_axi_wdata == 0xFF0055AA);
        REQUIRE(t.m_mem_axi_wlast == 1);

        clk(t);

        // STORE DATA ////////////////////////
        // Output contains the data from all elemets except the last
        for (uint32_t i = 0; i < BEATS_PER_TRANSFER - 1; i++)
        {
            REQUIRE(t.s_cmd_axis_tready == 0);

            REQUIRE(t.m_mem_axi_wvalid == 1);
            REQUIRE(t.m_mem_axi_wdata == 0xFF0055AA);
            REQUIRE(t.m_mem_axi_wlast == 0);

            clk(t);
        }

        // Second transfer
        // STORE DATA ////////////////////////
        // Check the last element
        REQUIRE(t.s_cmd_axis_tready == 0);

        REQUIRE(t.m_mem_axi_wvalid == 1);
        REQUIRE(t.m_mem_axi_wdata == 0xFF0055AA);
        REQUIRE(t.m_mem_axi_wlast == 1);

        clk(t);

        // IDLE /////////////////////////////
        REQUIRE(t.m_mem_axi_awvalid == 0);
        REQUIRE(t.m_mem_axi_wvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}

TEST_CASE("Load data simple", "[Memory]")
{
    VDmaStreamEngine* top = new VDmaStreamEngine();
    VDmaStreamEngine& t = *top;

    reset(t);
    static constexpr uint32_t OP = 0x2000'0000;
    static constexpr uint32_t SIZE = 0x0000'0100; 
    static constexpr uint32_t COMMAND = OP | SIZE; // Stream 256 bytes from one stream interface to another
    static constexpr uint32_t BEATS_PER_TRANSFER = 128 / 4;

    // Tell the command parser that we are ready to receive data
    t.m_cmd_axis_tready = 1;
    t.m_mem_axi_arready = 1;


    t.m_mem_axi_rready = 0;

    // Run this test two times, so that we know that we can start a new transfer after the old one
    for (uint32_t i = 0; i < 1; i++) 
    {
        // COMMAND ///////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 0);
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = COMMAND;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // COMMAND ADDR //////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 0);
        REQUIRE(t.m_cmd_axis_tvalid == 0);
        
        REQUIRE(t.s_cmd_axis_tready == 1);

        // Inputs
        // Input command
        t.s_cmd_axis_tdata = 0x100;
        t.s_cmd_axis_tvalid = 1;

        // Process command
        clk(t);

        // First test the address channel (data and address are independent)
        // LOAD ADDR /////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 0);
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        t.s_cmd_axis_tvalid = 0;

        clk(t);

        // LOAD ADDR /////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 1);
        REQUIRE(t.m_mem_axi_araddr == 0x100);
        
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // LOAD ADDR /////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 1);
        REQUIRE(t.m_mem_axi_araddr == 0x180);
        
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);

        // LOAD ADDR /////////////////////////
        // Outputs
        REQUIRE(t.m_mem_axi_arvalid == 0);
        
        REQUIRE(t.m_mem_axi_rvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        REQUIRE(t.m_cmd_axis_tvalid == 0);

        clk(t);

        // Now check the data channel
        // LOAD DATA /////////////////////////
        // Initial value
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        REQUIRE(t.m_mem_axi_rready == 1);

        t.m_mem_axi_rvalid = 1;
        t.m_mem_axi_rdata = 0;

        clk(t);

        // LOAD DATA /////////////////////////
        // Output contains the data from all elemets except the last
        for (uint32_t i = 1; i < BEATS_PER_TRANSFER * 2; i++)
        {
            REQUIRE(t.m_mem_axi_rready == 1);

            REQUIRE(t.m_cmd_axis_tvalid == 1);
            REQUIRE(t.m_cmd_axis_tdata == i - 1);
            REQUIRE(t.m_cmd_axis_tlast == 0);

            t.m_mem_axi_rdata = i;

            clk(t);
        }

        // LOAD DATA /////////////////////////
        // Check the last element
        REQUIRE(t.m_mem_axi_rready == 0);

        REQUIRE(t.m_cmd_axis_tvalid == 1);
        REQUIRE(t.m_cmd_axis_tdata == 63);
        REQUIRE(t.m_cmd_axis_tlast == 1);

        clk(t);

        // IDLE /////////////////////////////
        REQUIRE(t.m_mem_axi_arvalid == 0);
        REQUIRE(t.m_cmd_axis_tvalid == 0);

        REQUIRE(t.s_cmd_axis_tready == 0);

        clk(t);
    }

    // Final model cleanup
    top->final();

    // Destroy model
    delete top;
}
