#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include "codec/BundleViewV7.h"
#include <iostream>
#include <string>
#include <inttypes.h>
#include <vector>
#include "Uri.h"
#include <boost/next_prior.hpp>
#include <boost/make_unique.hpp>

static const uint64_t PRIMARY_SRC_NODE = 100;
static const uint64_t PRIMARY_SRC_SVC = 1;
static const uint64_t PRIMARY_DEST_NODE = 200;
static const uint64_t PRIMARY_DEST_SVC = 2;
static const uint64_t PRIMARY_TIME = 10000;
static const uint64_t PRIMARY_LIFETIME = 2000;
static const uint64_t PRIMARY_SEQ = 1;

static void AppendCanonicalBlockAndRender(BundleViewV7 & bv, uint8_t newType, std::string & newBlockBody, uint64_t blockNumber, const uint8_t crcTypeToUse) {
    //std::cout << "append " << (int)newType << "\n";
    std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7CanonicalBlock>();
    Bpv7CanonicalBlock & block = *blockPtr;
    block.m_blockTypeCode = newType;
    block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
    block.m_dataLength = newBlockBody.size();
    block.m_dataPtr = (uint8_t*)newBlockBody.data(); //blockBodyAsVecUint8 must remain in scope until after render
    block.m_crcType = crcTypeToUse;
    block.m_blockNumber = blockNumber;
    bv.AppendMoveCanonicalBlock(blockPtr);
    BOOST_REQUIRE(bv.Render(5000));
}
static void PrependCanonicalBlockAndRender(BundleViewV7 & bv, uint8_t newType, std::string & newBlockBody, uint64_t blockNumber, const uint8_t crcTypeToUse) {
    std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7CanonicalBlock>();
    Bpv7CanonicalBlock & block = *blockPtr;
    block.m_blockTypeCode = newType;
    block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
    block.m_dataLength = newBlockBody.size();
    block.m_dataPtr = (uint8_t*)newBlockBody.data(); //blockBodyAsVecUint8 must remain in scope until after render
    block.m_crcType = crcTypeToUse;
    block.m_blockNumber = blockNumber;
    bv.PrependMoveCanonicalBlock(blockPtr);
    BOOST_REQUIRE(bv.Render(5000));
}
static void PrependCanonicalBlockAndRender_AllocateOnly(BundleViewV7 & bv, uint8_t newType, uint64_t dataLengthToAllocate, uint64_t blockNumber, const uint8_t crcTypeToUse) {
    //std::cout << "append " << (int)newType << "\n";
    std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7CanonicalBlock>();
    Bpv7CanonicalBlock & block = *blockPtr;
    block.m_blockTypeCode = newType;
    block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
    block.m_dataLength = dataLengthToAllocate;
    block.m_dataPtr = NULL;
    block.m_crcType = crcTypeToUse;
    block.m_blockNumber = blockNumber;
    bv.PrependMoveCanonicalBlock(blockPtr);
    BOOST_REQUIRE(bv.Render(5000));
}

static void ChangeCanonicalBlockAndRender(BundleViewV7 & bv, uint8_t oldType, uint8_t newType, std::string & newBlockBody) {

    //std::cout << "change " << (int)oldType << " to " << (int)newType << "\n";
    std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
    bv.GetCanonicalBlocksByType(oldType, blocks);
    BOOST_REQUIRE_EQUAL(blocks.size(), 1);
    Bpv7CanonicalBlock & block = *(blocks[0]->headerPtr);
    block.m_blockTypeCode = newType;
    block.m_dataLength = newBlockBody.size();
    block.m_dataPtr = (uint8_t*)newBlockBody.data(); //blockBodyAsVecUint8 must remain in scope until after render
    blocks[0]->SetManuallyModified();

    BOOST_REQUIRE(bv.Render(5000));

}

static void GenerateBundle(const std::vector<uint8_t> & canonicalTypesVec, const std::vector<std::string> & canonicalBodyStringsVec, BundleViewV7 & bv, const uint8_t crcTypeToUse) {

    Bpv7CbhePrimaryBlock & primary = bv.m_primaryBlockView.header;
    primary.SetZero();


    primary.m_bundleProcessingControlFlags = BPV7_BUNDLEFLAG_NOFRAGMENT;  //All BP endpoints identified by ipn-scheme endpoint IDs are singleton endpoints.
    primary.m_sourceNodeId.Set(PRIMARY_SRC_NODE, PRIMARY_SRC_SVC);
    primary.m_destinationEid.Set(PRIMARY_DEST_NODE, PRIMARY_DEST_SVC);
    primary.m_reportToEid.Set(0, 0);
    primary.m_creationTimestamp.millisecondsSinceStartOfYear2000 = PRIMARY_TIME;
    primary.m_lifetimeMilliseconds = PRIMARY_LIFETIME;
    primary.m_creationTimestamp.sequenceNumber = PRIMARY_SEQ;
    primary.m_crcType = crcTypeToUse;
    bv.m_primaryBlockView.SetManuallyModified();

    for (std::size_t i = 0; i < canonicalTypesVec.size(); ++i) {
        std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7CanonicalBlock>();
        Bpv7CanonicalBlock & block = *blockPtr;
        //block.SetZero();

        block.m_blockTypeCode = canonicalTypesVec[i];
        block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
        block.m_blockNumber = i;
        block.m_crcType = crcTypeToUse;
        const std::string & blockBody = canonicalBodyStringsVec[i];
        block.m_dataLength = blockBody.size();
        block.m_dataPtr = (uint8_t*)blockBody.data(); //blockBodyAsVecUint8 must remain in scope until after render
        BOOST_REQUIRE(blockPtr);
        bv.AppendMoveCanonicalBlock(blockPtr);
        BOOST_REQUIRE(!blockPtr);
    }

    BOOST_REQUIRE(bv.Render(5000));
}

BOOST_AUTO_TEST_CASE(BundleViewV7TestCase)
{
    const std::vector<uint8_t> canonicalTypesVec = { 5,3,2,BPV7_BLOCKTYPE_PAYLOAD }; //last block must be payload block
    const std::vector<std::string> canonicalBodyStringsVec = { "The ", "quick ", " brown", " fox" };
    const std::vector<uint8_t> crcTypesVec = { BPV7_CRC_TYPE_NONE, BPV7_CRC_TYPE_CRC16_X25, BPV7_CRC_TYPE_CRC32C }; //last block must be payload block
    for (std::size_t crcI = 0; crcI < crcTypesVec.size(); ++crcI) {
        const uint8_t crcTypeToUse = crcTypesVec[crcI];

        BundleViewV7 bv;
        GenerateBundle(canonicalTypesVec, canonicalBodyStringsVec, bv, crcTypeToUse);
        std::vector<uint8_t> bundleSerializedOriginal(bv.m_frontBuffer);
        //std::cout << "renderedsize: " << bv.m_frontBuffer.size() << "\n";

        BOOST_REQUIRE_GT(bundleSerializedOriginal.size(), 0);
        std::vector<uint8_t> bundleSerializedCopy(bundleSerializedOriginal); //the copy can get modified by bundle view on first load
        BOOST_REQUIRE(bundleSerializedOriginal == bundleSerializedCopy);
        bv.Reset();
        //std::cout << "sz " << bundleSerializedCopy.size() << std::endl;
        BOOST_REQUIRE(bv.LoadBundle(&bundleSerializedCopy[0], bundleSerializedCopy.size()));
        BOOST_REQUIRE(bv.m_backBuffer != bundleSerializedCopy);
        BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedCopy);

        Bpv7CbhePrimaryBlock & primary = bv.m_primaryBlockView.header;
        BOOST_REQUIRE_EQUAL(primary.m_sourceNodeId, cbhe_eid_t(PRIMARY_SRC_NODE, PRIMARY_SRC_SVC));
        BOOST_REQUIRE_EQUAL(primary.m_destinationEid, cbhe_eid_t(PRIMARY_DEST_NODE, PRIMARY_DEST_SVC));
        BOOST_REQUIRE_EQUAL(primary.m_creationTimestamp, TimestampUtil::bpv7_creation_timestamp_t(PRIMARY_TIME, PRIMARY_SEQ));
        BOOST_REQUIRE_EQUAL(primary.m_lifetimeMilliseconds, PRIMARY_LIFETIME);

        BOOST_REQUIRE_EQUAL(bv.GetNumCanonicalBlocks(), canonicalTypesVec.size());
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(10), 0);
        for (std::size_t i = 0; i < canonicalTypesVec.size(); ++i) {
            BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(canonicalTypesVec[i]), 1);
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(canonicalTypesVec[i], blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            const char * strPtr = (const char *)blocks[0]->headerPtr->m_dataPtr;
            std::string s(strPtr, strPtr + blocks[0]->headerPtr->m_dataLength);
            BOOST_REQUIRE_EQUAL(s, canonicalBodyStringsVec[i]);
            BOOST_REQUIRE_EQUAL(blocks[0]->headerPtr->m_blockTypeCode, canonicalTypesVec[i]);
        }

        BOOST_REQUIRE(bv.Render(5000));
        BOOST_REQUIRE(bv.m_backBuffer != bundleSerializedCopy);
        BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedCopy.size());
        BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedCopy);

        //change 2nd block from quick to slow and type from 3 to 6 and render
        const uint32_t quickCrc = (crcTypeToUse == BPV7_CRC_TYPE_NONE) ? 0 : 
            (crcTypeToUse == BPV7_CRC_TYPE_CRC16_X25) ? boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc16 :
            boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc32;
        ChangeCanonicalBlockAndRender(bv, 3, 6, std::string("slow "));
        const uint32_t slowCrc = (crcTypeToUse == BPV7_CRC_TYPE_NONE) ? 1 :
            (crcTypeToUse == BPV7_CRC_TYPE_CRC16_X25) ? boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc16 :
            boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc32;
        BOOST_REQUIRE_NE(quickCrc, slowCrc);
        BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bv.m_backBuffer.size() - 1); //"quick" to "slow"
        BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedOriginal);
        BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bv.m_renderedBundle.size());
        BOOST_REQUIRE_EQUAL(bv.GetNumCanonicalBlocks(), canonicalTypesVec.size());

        //Render again
        //std::cout << "render again\n";
        BOOST_REQUIRE(bv.Render(5000));
        BOOST_REQUIRE(bv.m_frontBuffer == bv.m_backBuffer);

        //revert 2nd block
        ChangeCanonicalBlockAndRender(bv, 6, 3, std::string("quick "));
        const uint32_t quickCrc2 = (crcTypeToUse == BPV7_CRC_TYPE_NONE) ? 0 :
            (crcTypeToUse == BPV7_CRC_TYPE_CRC16_X25) ? boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc16 :
            boost::next(bv.m_listCanonicalBlockView.begin())->headerPtr->m_computedCrc32;
        //std::cout << "crc=" << quickCrc << "\n";
        BOOST_REQUIRE_EQUAL(quickCrc, quickCrc2);
        BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size());
        BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bv.m_renderedBundle.size());
        BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedOriginal);




        {
            //change PRIMARY_SEQ from 1 to 65539 (adding 4 bytes)
            bv.m_primaryBlockView.header.m_creationTimestamp.sequenceNumber = 65539;
            bv.m_primaryBlockView.SetManuallyModified();
            BOOST_REQUIRE(bv.m_primaryBlockView.dirty);
            //std::cout << "render increase primary seq\n";
            BOOST_REQUIRE(bv.Render(5000));
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size() + 4);
            BOOST_REQUIRE(!bv.m_primaryBlockView.dirty); //render removed dirty
            BOOST_REQUIRE_EQUAL(primary.m_lifetimeMilliseconds, PRIMARY_LIFETIME);
            BOOST_REQUIRE_EQUAL(primary.m_creationTimestamp.sequenceNumber, 65539);

            //restore PRIMARY_SEQ
            bv.m_primaryBlockView.header.m_creationTimestamp.sequenceNumber = PRIMARY_SEQ;
            bv.m_primaryBlockView.SetManuallyModified();
            //std::cout << "render restore primary seq\n";
            BOOST_REQUIRE(bv.Render(5000));
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size());
            BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedOriginal); //back to equal
        }

        //delete and re-add 1st block
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(canonicalTypesVec.front(), blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            blocks[0]->markedForDeletion = true;
            //std::cout << "render delete last block\n";
            BOOST_REQUIRE(bv.Render(5000));
            BOOST_REQUIRE_EQUAL(bv.GetNumCanonicalBlocks(), canonicalTypesVec.size() - 1);
            const uint64_t canonicalSize = //uint64_t bufferSize
                1 + //cbor initial byte denoting cbor array
                1 + //block type code byte
                1 + //block number
                1 + //m_blockProcessingControlFlags
                1 + //crc type code byte
                1 + //byte string header
                canonicalBodyStringsVec.front().length() + //data = len("The ")
                ((crcTypeToUse == BPV7_CRC_TYPE_NONE) ? 0 : (crcTypeToUse == BPV7_CRC_TYPE_CRC16_X25) ? 3 : 5); //crc byte array
            //std::cout << "canonicalSize=" << canonicalSize << "\n";
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size() - canonicalSize);

            PrependCanonicalBlockAndRender(bv, canonicalTypesVec.front(), std::string(canonicalBodyStringsVec.front()), 0, crcTypeToUse); //0 was block number 0 from GenerateBundle
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size());
            BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedOriginal); //back to equal
        }

        //delete and re-add 1st block by preallocation
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(canonicalTypesVec.front(), blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            blocks[0]->markedForDeletion = true;
            //std::cout << "render delete last block\n";
            BOOST_REQUIRE(bv.Render(5000));
            BOOST_REQUIRE_EQUAL(bv.GetNumCanonicalBlocks(), canonicalTypesVec.size() - 1);
            const uint64_t canonicalSize = //uint64_t bufferSize
                1 + //cbor initial byte denoting cbor array
                1 + //block type code byte
                1 + //block number
                1 + //m_blockProcessingControlFlags
                1 + //crc type code byte
                1 + //byte string header
                canonicalBodyStringsVec.front().length() + //data = len("The ")
                ((crcTypeToUse == BPV7_CRC_TYPE_NONE) ? 0 : (crcTypeToUse == BPV7_CRC_TYPE_CRC16_X25) ? 3 : 5); //crc byte array
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size() - canonicalSize);

            bv.m_backBuffer.assign(bv.m_backBuffer.size(), 0); //make sure zeroed out
            PrependCanonicalBlockAndRender_AllocateOnly(bv, canonicalTypesVec.front(), canonicalBodyStringsVec.front().length(), 0, crcTypeToUse); //0 was block number 0 from GenerateBundle
            BOOST_REQUIRE_EQUAL(bv.m_frontBuffer.size(), bundleSerializedOriginal.size());
            BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedOriginal); //still not equal, need to copy data
            bv.GetCanonicalBlocksByType(canonicalTypesVec.front(), blocks); //get new preallocated block
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            BOOST_REQUIRE_EQUAL(blocks[0]->headerPtr->m_dataLength, canonicalBodyStringsVec.front().length()); //make sure preallocated
            BOOST_REQUIRE_EQUAL((unsigned int)blocks[0]->headerPtr->m_dataPtr[0], 0); //make sure was zeroed out from above
            memcpy(blocks[0]->headerPtr->m_dataPtr, canonicalBodyStringsVec.front().data(), canonicalBodyStringsVec.front().length()); //copy data
            if (crcTypeToUse != BPV7_CRC_TYPE_NONE) {
                BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedOriginal); //still not equal, need to recompute crc
                blocks[0]->headerPtr->RecomputeCrcAfterDataModification((uint8_t*)blocks[0]->actualSerializedBlockPtr.data(), blocks[0]->actualSerializedBlockPtr.size()); //recompute crc
            }
            BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedOriginal); //back to equal
        }

        //test loads.
        {
            BOOST_REQUIRE(bundleSerializedCopy == bundleSerializedOriginal); //back to equal
            BOOST_REQUIRE(bv.CopyAndLoadBundle(&bundleSerializedCopy[0], bundleSerializedCopy.size())); //calls reset
            BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedCopy);
            BOOST_REQUIRE(bv.SwapInAndLoadBundle(bundleSerializedCopy)); //calls reset
            BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedCopy);
            BOOST_REQUIRE(bv.m_frontBuffer == bundleSerializedOriginal);
        }
    }



}



BOOST_AUTO_TEST_CASE(Bpv7ExtensionBlocksTestCase)
{
    static const uint64_t PREVIOUS_NODE = 12345;
    static const uint64_t PREVIOUS_SVC = 678910;
    static const uint64_t BUNDLE_AGE_MS = 135791113;
    static const uint8_t HOP_LIMIT = 250;
    static const uint8_t HOP_COUNT = 200;
    const std::string payloadString = { "This is the data inside the bpv7 payload block!!!" };
    const std::vector<uint8_t> canonicalTypesVec = { 5,3,2,BPV7_BLOCKTYPE_PAYLOAD }; //last block must be payload block
    
    const std::vector<uint8_t> crcTypesVec = { BPV7_CRC_TYPE_NONE, BPV7_CRC_TYPE_CRC16_X25, BPV7_CRC_TYPE_CRC32C }; //last block must be payload block
    for (std::size_t crcI = 0; crcI < crcTypesVec.size(); ++crcI) {
        const uint8_t crcTypeToUse = crcTypesVec[crcI];

        BundleViewV7 bv;
        Bpv7CbhePrimaryBlock & primary = bv.m_primaryBlockView.header;
        primary.SetZero();


        primary.m_bundleProcessingControlFlags = BPV7_BUNDLEFLAG_NOFRAGMENT;  //All BP endpoints identified by ipn-scheme endpoint IDs are singleton endpoints.
        primary.m_sourceNodeId.Set(PRIMARY_SRC_NODE, PRIMARY_SRC_SVC);
        primary.m_destinationEid.Set(PRIMARY_DEST_NODE, PRIMARY_DEST_SVC);
        primary.m_reportToEid.Set(0, 0);
        primary.m_creationTimestamp.millisecondsSinceStartOfYear2000 = PRIMARY_TIME;
        primary.m_lifetimeMilliseconds = PRIMARY_LIFETIME;
        primary.m_creationTimestamp.sequenceNumber = PRIMARY_SEQ;
        primary.m_crcType = crcTypeToUse;
        bv.m_primaryBlockView.SetManuallyModified();

        //add previous node block
        {
            std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7PreviousNodeCanonicalBlock>();
            Bpv7PreviousNodeCanonicalBlock & block = *(reinterpret_cast<Bpv7PreviousNodeCanonicalBlock*>(blockPtr.get()));
            //block.SetZero();

            block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
            block.m_blockNumber = 1;
            block.m_crcType = crcTypeToUse;
            block.m_previousNode.Set(PREVIOUS_NODE, PREVIOUS_SVC);
            bv.AppendMoveCanonicalBlock(blockPtr);
        }

        //add bundle age block
        {
            std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7BundleAgeCanonicalBlock>();
            Bpv7BundleAgeCanonicalBlock & block = *(reinterpret_cast<Bpv7BundleAgeCanonicalBlock*>(blockPtr.get()));
            //block.SetZero();

            block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
            block.m_blockNumber = 2;
            block.m_crcType = crcTypeToUse;
            block.m_bundleAgeMilliseconds = BUNDLE_AGE_MS;
            bv.AppendMoveCanonicalBlock(blockPtr);
        }

        //add hop count block
        {
            std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7HopCountCanonicalBlock>();
            Bpv7HopCountCanonicalBlock & block = *(reinterpret_cast<Bpv7HopCountCanonicalBlock*>(blockPtr.get()));
            //block.SetZero();

            block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
            block.m_blockNumber = 3;
            block.m_crcType = crcTypeToUse;
            block.m_hopLimit = HOP_LIMIT;
            block.m_hopCount = HOP_COUNT;
            bv.AppendMoveCanonicalBlock(blockPtr);
        }
        
        //add payload block
        {
            
            std::unique_ptr<Bpv7CanonicalBlock> blockPtr = boost::make_unique<Bpv7CanonicalBlock>();
            Bpv7CanonicalBlock & block = *blockPtr;
            //block.SetZero();

            block.m_blockTypeCode = BPV7_BLOCKTYPE_PAYLOAD;
            block.m_blockProcessingControlFlags = BPV7_BLOCKFLAG_REMOVE_BLOCK_IF_IT_CANT_BE_PROCESSED; //something for checking against
            block.m_blockNumber = 4;
            block.m_crcType = crcTypeToUse;
            block.m_dataLength = payloadString.size();
            block.m_dataPtr = (uint8_t*)payloadString.data(); //payloadString must remain in scope until after render
            bv.AppendMoveCanonicalBlock(blockPtr);

        }

        BOOST_REQUIRE(bv.Render(5000));

        std::vector<uint8_t> bundleSerializedOriginal(bv.m_frontBuffer);
        //std::cout << "renderedsize: " << bv.m_frontBuffer.size() << "\n";

        BOOST_REQUIRE_GT(bundleSerializedOriginal.size(), 0);
        std::vector<uint8_t> bundleSerializedCopy(bundleSerializedOriginal); //the copy can get modified by bundle view on first load
        BOOST_REQUIRE(bundleSerializedOriginal == bundleSerializedCopy);
        bv.Reset();
        //std::cout << "sz " << bundleSerializedCopy.size() << std::endl;
        BOOST_REQUIRE(bv.LoadBundle(&bundleSerializedCopy[0], bundleSerializedCopy.size()));
        BOOST_REQUIRE(bv.m_backBuffer != bundleSerializedCopy);
        BOOST_REQUIRE(bv.m_frontBuffer != bundleSerializedCopy);

        BOOST_REQUIRE_EQUAL(primary.m_sourceNodeId, cbhe_eid_t(PRIMARY_SRC_NODE, PRIMARY_SRC_SVC));
        BOOST_REQUIRE_EQUAL(primary.m_destinationEid, cbhe_eid_t(PRIMARY_DEST_NODE, PRIMARY_DEST_SVC));
        BOOST_REQUIRE_EQUAL(primary.m_creationTimestamp, TimestampUtil::bpv7_creation_timestamp_t(PRIMARY_TIME, PRIMARY_SEQ));
        BOOST_REQUIRE_EQUAL(primary.m_lifetimeMilliseconds, PRIMARY_LIFETIME);

        BOOST_REQUIRE_EQUAL(bv.GetNumCanonicalBlocks(), 4);
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(BPV7_BLOCKTYPE_PREVIOUS_NODE), 1);
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(BPV7_BLOCKTYPE_BUNDLE_AGE), 1);
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(BPV7_BLOCKTYPE_HOP_COUNT), 1);
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(BPV7_BLOCKTYPE_PAYLOAD), 1);
        BOOST_REQUIRE_EQUAL(bv.GetCanonicalBlockCountByType(100), 0);
        
        //get previous node
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(BPV7_BLOCKTYPE_PREVIOUS_NODE, blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            Bpv7PreviousNodeCanonicalBlock* previousNodeBlockPtr = dynamic_cast<Bpv7PreviousNodeCanonicalBlock*>(blocks[0]->headerPtr.get());
            BOOST_REQUIRE(previousNodeBlockPtr);
            BOOST_REQUIRE_EQUAL(previousNodeBlockPtr->m_blockTypeCode, BPV7_BLOCKTYPE_PREVIOUS_NODE);
            BOOST_REQUIRE_EQUAL(previousNodeBlockPtr->m_blockNumber, 1);
            BOOST_REQUIRE_EQUAL(previousNodeBlockPtr->m_previousNode.nodeId, PREVIOUS_NODE);
            BOOST_REQUIRE_EQUAL(previousNodeBlockPtr->m_previousNode.serviceId, PREVIOUS_SVC);
        }

        //get bundle age
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(BPV7_BLOCKTYPE_BUNDLE_AGE, blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            Bpv7BundleAgeCanonicalBlock* bundleAgeBlockPtr = dynamic_cast<Bpv7BundleAgeCanonicalBlock*>(blocks[0]->headerPtr.get());
            BOOST_REQUIRE(bundleAgeBlockPtr);
            BOOST_REQUIRE_EQUAL(bundleAgeBlockPtr->m_blockTypeCode, BPV7_BLOCKTYPE_BUNDLE_AGE);
            BOOST_REQUIRE_EQUAL(bundleAgeBlockPtr->m_blockNumber, 2);
            BOOST_REQUIRE_EQUAL(bundleAgeBlockPtr->m_bundleAgeMilliseconds, BUNDLE_AGE_MS);
        }

        //get hop count
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(BPV7_BLOCKTYPE_HOP_COUNT, blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            Bpv7HopCountCanonicalBlock* hopCountBlockPtr = dynamic_cast<Bpv7HopCountCanonicalBlock*>(blocks[0]->headerPtr.get());
            BOOST_REQUIRE(hopCountBlockPtr);
            BOOST_REQUIRE_EQUAL(hopCountBlockPtr->m_blockTypeCode, BPV7_BLOCKTYPE_HOP_COUNT);
            BOOST_REQUIRE_EQUAL(hopCountBlockPtr->m_blockNumber, 3);
            BOOST_REQUIRE_EQUAL(hopCountBlockPtr->m_hopLimit, HOP_LIMIT);
            BOOST_REQUIRE_EQUAL(hopCountBlockPtr->m_hopCount, HOP_COUNT);
        }
        
        //get payload
        {
            std::vector<BundleViewV7::Bpv7CanonicalBlockView*> blocks;
            bv.GetCanonicalBlocksByType(BPV7_BLOCKTYPE_PAYLOAD, blocks);
            BOOST_REQUIRE_EQUAL(blocks.size(), 1);
            const char * strPtr = (const char *)blocks[0]->headerPtr->m_dataPtr;
            std::string s(strPtr, strPtr + blocks[0]->headerPtr->m_dataLength);
            //std::cout << "s: " << s << "\n";
            BOOST_REQUIRE_EQUAL(s, payloadString);
            BOOST_REQUIRE_EQUAL(blocks[0]->headerPtr->m_blockTypeCode, BPV7_BLOCKTYPE_PAYLOAD);
            BOOST_REQUIRE_EQUAL(blocks[0]->headerPtr->m_blockNumber, 4);
        }
    }
}

