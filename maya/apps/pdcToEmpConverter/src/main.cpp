#include <math.h>
#include <sstream>
#include <fstream>

#include <Ng/NgBody.h>
#include <em/em_block3_array.h>

#include "endianSwap.h"

struct PDCHeader
{
    char format[4];
    int formatVersion;
    int byteOrder;
    int extra1;
    int extra2;
    int numParticles;
    int numAttributes;
};

int main( int argc, char *argv[] )
{	
    if ( argc < 3 )
    {
        std::cout << "Please supply the inputpath and the outputpath" << std::endl;
        return 0;
    }

    std::string inputPath = std::string(argv[1]);
    std::string outputPath = std::string(argv[2]);

    std::ifstream pIn( inputPath.c_str(), std::ios::binary );

    PDCHeader fileHeader;
    pIn.read( (char*)&fileHeader, sizeof(struct PDCHeader) );
    swap_endianity( fileHeader.byteOrder );

    //Do we need to swap the byteorder?
    if(fileHeader.byteOrder!=0)
    {
        swap_endianity( fileHeader.formatVersion );
        swap_endianity( fileHeader.extra1 );
        swap_endianity( fileHeader.extra2 );
        swap_endianity( fileHeader.numParticles );
        swap_endianity( fileHeader.numAttributes );
    }

    std::cout << "Format :" << fileHeader.format  << std::endl;
    std::cout << "Version :" << fileHeader.formatVersion  << std::endl;
    std::cout << "byteOrder :" << fileHeader.byteOrder  << std::endl;
    std::cout << "extra1 :" << fileHeader.extra1  << std::endl;
    std::cout << "extra2 :" << fileHeader.extra2  << std::endl;
    std::cout << "numParticles :" << fileHeader.numParticles  << std::endl;
    std::cout << "numAttributes :" << fileHeader.numAttributes  << std::endl;

    //Create the naiad Body
    Ng::Body * body;
    Ng::ParticleShape* psh;
    try
    {
        // Create Naiad Body
        body = new Ng::Body( "particles" );
        body->match("particle");

        psh = &body->mutableParticleShape();
    }
    catch(std::exception& e) {
        std::cerr << "naiad_PTC_TO_EMP::main()" << e.what() << std::endl;
    }

    Ng::TileLayout& layout(body->mutableLayout());
    // PLEASE NOTE: this should really be a box close to a particle, instead of (0,0,0)...(1,1,1) but I was in a hurry!
    // so this will make some "dummy" tiles...
    layout.worldBoxRefine(em::vec3f(0,0,0), em::vec3f(1,1,1),1,false);
    // this will add blocks where there are tiles... (we are synching the particle-shape's blocks to the tile-layout)
    psh->sync(layout);

    int nameLength, dataType, perObjectInt, perObjectDouble;

    // Loop the attributes
    for ( int i(0); i < fileHeader.numAttributes; ++i )
    {
        //How long is the attribute name
        pIn.read( (char*)&nameLength, sizeof(int) );
        if(fileHeader.byteOrder!=0) swap_endianity( nameLength );

        //Read the attribute name
        char* nameHolder = new char[nameLength+1];
        pIn.read( nameHolder, nameLength );
        nameHolder[nameLength] = '\0';

        //Read the attribute type
        pIn.read( (char*)&dataType, sizeof(int) );
        if(fileHeader.byteOrder!=0) swap_endianity( dataType );

        std::cout << "ChannelName :" << nameHolder << " with lenght: " << nameLength  << std::endl;
        delete [] nameHolder;

        switch ( dataType )
        {
        case 0 : //int
            {
                pIn.read( (char*)&perObjectInt, sizeof(int) );
                if(fileHeader.byteOrder!=0) swap_endianity( perObjectInt );
                break;

                //Create the property on the body
                body->createProp1i( nameHolder , "", "" );

                std::stringstream strStream;
                strStream << perObjectInt;

                //Get the property and assign
                body->prop1i(nameHolder)->setExpr( strStream.str() );
            }
        case 1 : //int array
            {
                //Create the channel in the particle shape
                psh->createChannel1f( std::string(nameHolder) , 0.0f );

                //Get the array
                em::block3_array1i& intBlocks(psh->mutableBlocks1i(nameHolder));
                em::block3i&    intData(intBlocks(0));
/*
                intData.resize( fileHeader.numParticles );
                pIn.read( (char*)(&intData[0]), fileHeader.numParticles*sizeof(int) );

                if(fileHeader.byteOrder!=0)
                {
                    for( em::array1i::iterator ittr=intData.begin(); ittr!=intData.end(); ++ittr)
                        swap_endianity( *ittr );
                }*/
            }
            break;
        case 2 : //double
            {
                pIn.read( (char*)&perObjectDouble, sizeof(double) );
                if(fileHeader.byteOrder!=0) swap_endianity( perObjectDouble );

                //Create the property on the body
                body->createProp1f( nameHolder , "", "" );

                std::stringstream strStream;
                strStream << perObjectDouble;

                //Get the property and assign
                body->prop1f(nameHolder)->setExpr( strStream.str() );
            }
            break;
        case 3 : //double array
            {
                std::vector<double> doubleArray(fileHeader.numParticles);
                pIn.read( (char*)(&doubleArray[0]), fileHeader.numParticles*sizeof(double) );

                if(fileHeader.byteOrder!=0)
                {
                    for( std::vector<double>::iterator ittr=doubleArray.begin(); ittr!=doubleArray.end(); ++ittr)
                        swap_endianity( *ittr );
                }

                //Create the channel in the particle shape
                psh->createChannel1f( std::string(nameHolder) , 0.0f );

                //Get the array
                em::block3_array1f& doubleBlocks(psh->mutableBlocks1f(nameHolder));
                em::block3f&    doubleData(doubleBlocks(0));

                //Transfer the data (casting it to a float)
                doubleData.reserve( doubleArray.size() );
                for( std::vector<double>::iterator ittr=doubleArray.begin(); ittr!=doubleArray.end(); ++ittr)
                {
                    doubleData.push_back( float(*ittr) );
                }
            }
            break;
        case 4 : //vector
            {
                pIn.read( (char*)&perObjectDouble, sizeof(double) );
                pIn.read( (char*)&perObjectDouble, sizeof(double) );
                pIn.read( (char*)&perObjectDouble, sizeof(double) );
                if(fileHeader.byteOrder!=0) swap_endianity( perObjectDouble );
            }
            break;
        case 5 : //vector array
            {
                std::vector<em::vec3d> doubleArray(fileHeader.numParticles);
                pIn.read( (char*)(&doubleArray[0][0]), fileHeader.numParticles*sizeof(double)*3 );

                if(fileHeader.byteOrder!=0)
                {
                    for( std::vector<em::vec3d>::iterator ittr=doubleArray.begin(); ittr!=doubleArray.end(); ++ittr)
                    {
                        swap_endianity( (*ittr)[0] );
                        swap_endianity( (*ittr)[1] );
                        swap_endianity( (*ittr)[2] );
                    }
                }

                //Create the channel in the particle shape
                psh->createChannel3f( std::string(nameHolder) , em::vec3f(0.0f,0.0f,0.0f) );

                em::block3_array3f& vectorBlocks(psh->mutableBlocks3f(nameHolder));
                em::block3vec3f&    vectorData(vectorBlocks(0));

                //Transfer the data (casting it to a float)
                vectorData.reserve( doubleArray.size() );
                em::vec3f floatVector;
                for( std::vector<em::vec3d>::iterator ittr=doubleArray.begin(); ittr!=doubleArray.end(); ++ittr)
                {
                    floatVector[0] = (*ittr)[0];
                    floatVector[1] = (*ittr)[1];
                    floatVector[2] = (*ittr)[2];
                    vectorData.push_back( floatVector );
                }
            }
            break;
        default :
                break;
    }
    }

    //Now that we are done writing.. order the data
    body->update();

    //finally write out the EMP file with the body
    try{
        Ng::EmpWriter empWriter( outputPath, 1.0 );
        Ng::String channels("*/*");
        empWriter.write( body , channels );
        empWriter.close();
    }
    catch(std::exception& e) {
        std::cerr << "naiad_PTC_TO_EMP::main()" << e.what() << std::endl;
    }


    // Save out the EMP file
    // saveEMP( myStream , outputPath );


    return 0;
}
