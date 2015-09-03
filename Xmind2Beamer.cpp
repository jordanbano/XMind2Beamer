#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include "boost/property_tree/ptree.hpp"
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

// compile me with:
//    cl /EHsc /I c:\local\boost_1_55_0 Xmind2Beamer.cpp

std::string m_keywordAppendix = "Appendix";
std::map< std::string, std::map< std::string, std::string> > m_styles;
std::vector< float > m_tabFontWidth;
std::vector< float > m_tabLineSize;
std::vector< float > m_tabBaselineSkip;
std::vector< float > m_tabSpace;
float m_sizeFrameTitle;
float m_slideHeight;
unsigned int m_numSubSlide;// no choice...
float m_thresholdSubsection=0.2;// ratio of number_of_slide_with_only_title/number_of_slides
float m_thresholdFigure=0.4;// how much we can scale the image to fit an available space

// we can fuse "small" sides between them 
// we have 3 modes:
// 0: don't fuse
// 1: fuse slides but not the slides that have been cut
// 2: fuse all the slides
int m_modeFuse=2;

void retrieveStyles( boost::property_tree::ptree _ptStyle )
{
	using boost::property_tree::ptree;

	_ptStyle = _ptStyle.get_child("xmap-styles");
	BOOST_FOREACH(ptree::value_type &vs, _ptStyle.get_child("styles"))// section
	{	
		if( vs.second.count("topic-properties") != 0 )
		{
			boost::property_tree::ptree attrs =  vs.second.get_child("topic-properties.<xmlattr>");
			
			// retrieve the font properties
			bool addStyle = false;
			std::map< std::string, std::string> newStyle;
			
			if( attrs.count("fo:color") )
			{
				newStyle.insert( std::pair<std::string,std::string>("color"
								 , attrs.get<std::string>("fo:color") ) );
				addStyle = true;						
			}
			
			if( attrs.count("fo:font-size") )
			{
				newStyle.insert( std::pair<std::string,std::string>("size"
								 , attrs.get<std::string>("fo:font-size") ) );
				addStyle = true;						
			}
			
			if( attrs.count("fo:font-style") )
			{
				newStyle.insert( std::pair<std::string,std::string>("style"
								 , attrs.get<std::string>("fo:font-style") ) );
				addStyle = true;						
			}
			
			if( attrs.count("fo:font-weight") )
			{
				newStyle.insert( std::pair<std::string,std::string>("weight"
								 , attrs.get<std::string>("fo:font-weight") ) );
				addStyle = true;						
			}
			
			if( addStyle )
			{
				m_styles.insert( std::pair< std::string,std::map< std::string,std::string > >(
								 vs.second.get<std::string>("<xmlattr>.id"), newStyle ) );
			}
		}
	}
}

// add a function to handle the special character
void formatText2Latex( std::string& _text )
{
	// create the symbol table
	std::vector< std::pair< std::string, std::string > > tableSymbol;
	tableSymbol.push_back(std::make_pair("\\","\\textbackslash"));// must be first
	tableSymbol.push_back(std::make_pair("é","\\'e"));
	tableSymbol.push_back(std::make_pair("è","\\`e"));
	tableSymbol.push_back(std::make_pair("ê","\\^e"));
	tableSymbol.push_back(std::make_pair("ë","\\\"e"));
	tableSymbol.push_back(std::make_pair("É","\\'E"));
	tableSymbol.push_back(std::make_pair("È","\\`E"));
	tableSymbol.push_back(std::make_pair("Ê","\\^E"));
	tableSymbol.push_back(std::make_pair("Ë","\\\"E"));
	tableSymbol.push_back(std::make_pair("à","\\`a"));
	tableSymbol.push_back(std::make_pair("â","\\^a"));
	tableSymbol.push_back(std::make_pair("À","\\`A"));
	tableSymbol.push_back(std::make_pair("Â","\\^A"));
	tableSymbol.push_back(std::make_pair("î","\\^i"));
	tableSymbol.push_back(std::make_pair("ï","\\\"\\i"));
	tableSymbol.push_back(std::make_pair("Î","\\^I"));
	tableSymbol.push_back(std::make_pair("Ï","\\\"\\I"));
	tableSymbol.push_back(std::make_pair("û","\\^u"));
	tableSymbol.push_back(std::make_pair("ù","\\`u"));
	tableSymbol.push_back(std::make_pair("Û","\\^U"));
	tableSymbol.push_back(std::make_pair("ô","\\^o"));
	tableSymbol.push_back(std::make_pair("Ô","\\^O"));
	tableSymbol.push_back(std::make_pair("ç","\\c c"));
	tableSymbol.push_back(std::make_pair("Ç","\\c C"));
	tableSymbol.push_back(std::make_pair("\"","''"));
	tableSymbol.push_back(std::make_pair("&","\\&"));
	tableSymbol.push_back(std::make_pair("$","\\$"));
	tableSymbol.push_back(std::make_pair("%","\\%"));
	tableSymbol.push_back(std::make_pair("{","\\{"));
	tableSymbol.push_back(std::make_pair("}","\\}"));
	tableSymbol.push_back(std::make_pair("_","\\_"));
	tableSymbol.push_back(std::make_pair("¶","\\P"));
	tableSymbol.push_back(std::make_pair("|","\\textbar"));
	tableSymbol.push_back(std::make_pair("–","\\textendash"));
	tableSymbol.push_back(std::make_pair("¡","\\textexclamdown"));
	tableSymbol.push_back(std::make_pair("£","\\pounds"));
	tableSymbol.push_back(std::make_pair("#","\\#"));
	tableSymbol.push_back(std::make_pair("§","\\S"));
	tableSymbol.push_back(std::make_pair("†","\\dag"));
	tableSymbol.push_back(std::make_pair("—","\\textemdash"));
	tableSymbol.push_back(std::make_pair("¿","\\textquestiondown"));
	tableSymbol.push_back(std::make_pair("<","\\textless"));
	tableSymbol.push_back(std::make_pair(">","\\textgreater"));
	tableSymbol.push_back(std::make_pair("€","\\euro"));
	tableSymbol.push_back(std::make_pair("™","$^{\\mbox{\\scriptsize{\\texttrademark}}}$ "));
	tableSymbol.push_back(std::make_pair("®","$^{\\mbox{\\scriptsize{\\textregistered}}}$ "));
	tableSymbol.push_back(std::make_pair("©","$^{\\mbox{\\scriptsize{\\copyright}}}$ "));
	
	// replace the symbols
	int posChar;
	for( int i=0; i < tableSymbol.size();++i )
	{
		std::string temp = tableSymbol[i].first;
		unsigned sizeSymbol = temp.size();
		
		int pos=0;
		
		while( _text.find(tableSymbol[i].first,pos) != std::string::npos )
		{
			posChar = _text.find(tableSymbol[i].first,pos);
			_text.replace(posChar,sizeSymbol,tableSymbol[i].second);
			
			pos = posChar + tableSymbol[i].second.size();
		}
	}
}

void fillSizeTable( )
{
	// measured manually based on the string "azertyuiopqsdfghjklmwxcvbn1234567890"
	// not accurate at all...
	m_tabFontWidth.push_back(0);
	m_tabFontWidth.push_back(6.61);// title
	m_tabFontWidth.push_back(5.3);// block 
	m_tabFontWidth.push_back(5.14);// itemize 1st depth
	m_tabFontWidth.push_back(4.72);// itemize 2nd depth
	m_tabFontWidth.push_back(4.33);// itemize 3rd depth
	
	// \linewidth
	m_tabLineSize.push_back(0);
	m_tabLineSize.push_back(342.2953);
	m_tabLineSize.push_back(342.2953);
	m_tabLineSize.push_back(320.39525);
	m_tabLineSize.push_back(298.4952);
	m_tabLineSize.push_back(276.59514);
	
	// \baselineskip
	m_tabBaselineSkip.push_back(0);
	m_tabBaselineSkip.push_back(18);
	m_tabBaselineSkip.push_back(14);
	m_tabBaselineSkip.push_back(13.6);
	m_tabBaselineSkip.push_back(12);
	m_tabBaselineSkip.push_back(11);
	
	// measured manually ...
	m_tabSpace.push_back(0);
	m_tabSpace.push_back(20);
	m_tabSpace.push_back(12);
	m_tabSpace.push_back(3);// \itemsep value
	m_tabSpace.push_back(1);
	m_tabSpace.push_back(0);
	
	// m_slideHeight = 273.14662;// \paperHeight result
	m_slideHeight = 255;// \paperHeight - footline
}

float countSizewithFontWidth(std::string _text, int _depth, float _fontWidth )
{	
	float size = (float) _text.size();// compute the size of the string 
	size *= _fontWidth;// multiply by the font size
	size = std::floor(size/m_tabLineSize[_depth]);// divide by the line size -> obtain the number of lines -1
	size++;// obtain the number of lines
	size *= m_tabBaselineSkip[_depth];// multiply by the line height
	size += m_tabSpace[_depth];// add additional space
	
	return size;
}

float countSize(std::string _text, int _depth )
{
	return countSizewithFontWidth( _text, _depth, m_tabFontWidth[_depth]);
}

float countSizeInsideSlide( boost::property_tree::ptree _pt, std::string _text, int _depth )
{	
	if(_depth > 5)
	{
		_depth = 5;
	}

	boost::property_tree::ptree attrs =  _pt.get_child("<xmlattr>");
	float fontWidth=m_tabFontWidth[_depth];
	bool bold=false;
	if( (attrs.count("style-id") != 0) )// have a specific font style
	{	
		std::map< std::string, std::map< std::string, std::string > >::iterator it;
		
		it = m_styles.find( attrs.get< std::string >("style-id") );
		if (it != m_styles.end())
		{
			std::map< std::string, std::string > styleMap = it->second;
			
			std::map< std::string, std::string >::iterator itStyle;
		    for(itStyle=styleMap.begin(); itStyle!=styleMap.end(); ++itStyle)
			{
				if( itStyle->first == "size")
				{			
					fontWidth = std::atof( itStyle->second.substr(0,itStyle->second.size()-2).c_str() );	// -2 for pt	
					fontWidth /= 2.14;// we estimate it with the default font that fontSize = 2.14*fontWidth
				}
				
				if( itStyle->first == "style" && itStyle->second == "italic" )
				{
					// do not change the font size
				}
				
				if( itStyle->first == "weight" && itStyle->second == "bold" )
				{
					// will add 9% 
				}
				
				if( itStyle->first == "color")
				{
					// do not change the font size
				}
			}
		}	
	}
	
	if( bold )
	{
		fontWidth *= 1.09;
	}

	return  countSizewithFontWidth( _text, _depth, fontWidth );
}

// manage if the text have a specific style or if is linked with a website link
void addText( std::ofstream& _latexFile, std::string _text )
{
	formatText2Latex( _text );
	_latexFile << _text;
}

void addTextInsideSlide( boost::property_tree::ptree _pt, std::ofstream& _latexFile, std::string _text, int _depth )
{	
	boost::property_tree::ptree attrs =  _pt.get_child("<xmlattr>");
	
	bool have_websitelink = (attrs.count("xlink:href") != 0);
	int have_style = (attrs.count("style-id") != 0);
	
	if( have_websitelink )// have a website link
	{
		// check if it's not a xmind link
		std::string link = attrs.get<std::string>("xlink:href");
		if( link.find("xmind") == std::string::npos )
		{
			_latexFile << "\\href{"<<attrs.get<std::string>("xlink:href")<<"}{";
			_latexFile << "\\textcolor{blue}{";
		}
		else
		{
			have_websitelink = false;
		}
	}
	
	if( have_style )// have a specific font style
	{
		have_style = 0;
		
		std::map< std::string, std::map< std::string, std::string > >::iterator it;
		
		it = m_styles.find( attrs.get< std::string >("style-id") );
		if (it != m_styles.end())
		{
			std::map< std::string, std::string > styleMap = it->second;
			
			std::map< std::string, std::string >::iterator itStyle;
		    for(itStyle=styleMap.begin(); itStyle!=styleMap.end(); ++itStyle)
			{
				if( itStyle->first == "size")
				{
					++have_style;
					
					_latexFile << "{\\fontsize{"<< itStyle->second <<"}{"<< itStyle->second <<"}\\selectfont ";
				}
				
				if( itStyle->first == "style" && itStyle->second == "italic" )
				{
					++have_style;
					
					_latexFile << "\\textit{";
				}
				
				if( itStyle->first == "weight" && itStyle->second == "bold" )
				{
					++have_style;
					
					_latexFile << "\\textbf{";
				}
				
				if( itStyle->first == "color")
				{
					++have_style;
					
					_latexFile << "\\textcolor[HTML]{"<< itStyle->second.erase(0,1) <<"}{";
				}
			}
		}
	}
		
	addText( _latexFile, _text );
	
	if( have_websitelink )// have a website link
	{
		_latexFile << "}}";
	}
	
	while( have_style )// have a specific font style
	{
		_latexFile << "}";
		
		--have_style;
	}
}

// cut a slide (ie close the tags (all the itemize, block and frame) and open new ones
// other idea: one slide with a overprint env. and \onslide<+-+($NbElement)> 
//             the issue is to obtain the number of elements before declaring the onslide...
//             moreover, one slide means one slide number for all the subslides which can be confusing
void cutSlide( std::ofstream& _latexFile , int _depth, std::vector< std::string >& _nameParent, bool _firstChildIsCut)
{
	if(_firstChildIsCut)
	{// we have to close the parent which have one less depth than the current node
		--_depth;
	}
	
	// LaTeX natively don't manage more than 3-depth list
	if( _depth > 3 )
	{
		_depth = 3;
	}
	// close all the itemize
	for(int i=_depth;i>0;--i)
	{
		std::string whitespaceTmp;
		for(int j=0;j<i;++j)
		{
			whitespaceTmp+="  ";
		}
		_latexFile<<whitespaceTmp<<"\\end{itemize}"<<std::endl;
	}
	
	// close the block and the frame
	_latexFile <<"\\end{block}"<<std::endl;
	_latexFile <<"\\end{frame}"<<std::endl;
	_latexFile <<std::endl;
	
	// update the parent name (X/Y) -> (X+1/Y)
	
	//remove X from the string
	std::string titleBlock = _nameParent[0];
	titleBlock = titleBlock.erase(titleBlock.find_last_of('(')+1,titleBlock.find_last_of('/')-titleBlock.find_last_of('(')-1);
	// put the X+1 inside the string
	char tempChar[2];
	sprintf(tempChar, "%d", m_numSubSlide );
	titleBlock = titleBlock.insert(titleBlock.find_last_of('(')+1,tempChar);

	// open a new frame/block
	_latexFile <<"\\begin{frame}{\\insertsectionhead:}"<<std::endl;
	_latexFile <<"\\begin{block}{";
	addText(_latexFile, titleBlock);
	_latexFile<<"}"<<std::endl;
	
	_nameParent[0] = titleBlock;
	
	if(_firstChildIsCut)
	{// now we deal with the child so we come back to the current depth
		++_depth;
	}
	
	// LaTeX natively don't manage more than 3-depth list
	if( _depth > 3 )
	{
		_depth = 3;
	}
	
	// open the itemize
	for(int i=0;i<_depth;++i)
	{		
		std::string whitespaceTmp;
		for(int j=i;j>=0;--j)
		{
			whitespaceTmp+="  ";
		}
		if( i == (_depth-1) )
		{
			_latexFile<<whitespaceTmp<<"\\begin{itemize}[<+->]"<<std::endl;
		}
		else
		{
			// latex needs an item before another begin{itemize}
			_latexFile<<whitespaceTmp<<"\\begin{itemize}[<+->]"<<std::endl;
			_latexFile<<whitespaceTmp<<"\\item<.->";
			addText(_latexFile,_nameParent[i+1]);
			_latexFile<<std::endl;
		}
	}
}

void exploreElementSlide( boost::property_tree::ptree& _pt, const float _spaceAvailable, float& _cptElement, int _depth, int& _nbSlides, float _cptParentName )
{			
	std::string text;

	if( _pt.count("children") != 0)
	{
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, _pt.get_child("children.topics")) 
		{
			if( v.first == "topic" )
			{
				float cptText=0.0;
				float cptFigure=0.0;
				
				if( v.second.get<std::string>("title") != "" )// for example, for an image without legend
				{
					text = v.second.get<std::string>("title");

					cptText = countSizeInsideSlide( v.second, text, _depth );
				}
				
				if( v.second.count("xhtml:img") != 0 )// this one have an image
				{		
					// check the image width and height
					int width  = v.second.get< int >("xhtml:img.<xmlattr>.svg:width");
					int height = v.second.get< int >("xhtml:img.<xmlattr>.svg:height");

					// check if the width of the image fit the block
					if( width > m_tabLineSize[_depth] )
					{
						// keep the aspect ratio
						height *= m_tabLineSize[_depth];
						height /= width;
						
						// change the width
						width = m_tabLineSize[_depth];		
					}

					// check the height of the image according to the height available
					if( height > (_spaceAvailable-_cptElement-cptText) )
					{
						// try to reduce the figure height to fit the remaining space in the slide
						if( (height*m_thresholdFigure) <= (_spaceAvailable-_cptElement-cptText) )
						{
							// keep the aspect ratio
							width *= (_spaceAvailable-_cptElement-cptText);
							width /= height;
						
							// change the height
							height = (_spaceAvailable-_cptElement-cptText);	
						}
						else
						{// the figure is just too big but cannot bigger than the max height (ie without any itemize)							
							int oldHeight = height;
							// change the height
							height = (height < (_spaceAvailable-cptText)) ? height : (_spaceAvailable-cptText);
							
							// keep the aspect ratio
							width *= height;
							width /= oldHeight;
						}
					}

					// save the new width/height inside the xml graph
					v.second.put("xhtml:img.<xmlattr>.svg:width", width);
					v.second.put("xhtml:img.<xmlattr>.svg:height", height);				
				
					cptFigure += height;
				}

				_cptElement += cptText + cptFigure;
				
				// cut slides: we add a "cut" node to the current element 
				// during LaTeX file writting, when we meet a "cut" node, we close the frame and open a new one
				if( _cptElement > _spaceAvailable )
				{
					// std::cout << text << " cpt= " << _cptParentName << std::endl;
					// std::cout << _spaceAvailable << " cpt= " << cptText << std::endl;
					
					v.second.put_child("cut", boost::property_tree::ptree() );
					
					_cptElement -= _spaceAvailable;
					++_nbSlides;
					
					// we have to add the parent name space
					_cptElement += _cptParentName;
				}
				
				if( _depth >= 6 )// we don't handle more than 3-depth itemize
				{
					cptText=0;
				}

				if( v.second.count("children") != 0)
				{
					exploreElementSlide( v.second, _spaceAvailable, _cptElement, _depth+1, _nbSlides,_cptParentName+cptText );
				}
			}
		}	
	}
}

void opFuseOrCutSlide( boost::property_tree::ptree& _pt )
{
	using boost::property_tree::ptree;
	
	float spaceAvailable = m_slideHeight-m_sizeFrameTitle;
	
	// count the size of each slide (ie which is a block) in the section/subsection
	std::vector< float > tabCpt;
	std::vector< int > tabSlides;
	BOOST_FOREACH(ptree::value_type &v, _pt.get_child("children.topics"))// a slide
	{
		if( v.first == "topic" )
		{
			std::string text = v.second.get< std::string >("title");
			float sizeBlockTitle = countSize(text,2);
			
			// check if the topic have an image
			if( v.second.count("xhtml:img") != 0 )
			{
				// if the topic have child...
				if( v.second.count("children") != 0 )
				{
					// ... we add the image to each children
					BOOST_FOREACH(ptree::value_type &v2, v.second.get_child("children.topics"))
					{
						v2.second.add_child("xhtml:img",v.second.get_child("xhtml:img"));
					}
					
					v.second.erase("xhtml:img");
				}
				else
				{
					// we add a new node with the image 
					v.second.add_child("children.topics.topic.xhtml:img",v.second.get_child("xhtml:img"));
					v.second.put("children.topics.topic.title","");
					
					v.second.erase("xhtml:img");
				}
			}
			
			float cpt = 0;
			int nbSlides = 1;
			float cptParentName=0.0;
			exploreElementSlide( v.second, (spaceAvailable-sizeBlockTitle), cpt, 3, nbSlides,cptParentName);
			
			// we will have to cut this slide
			if( nbSlides > 1 )
			{		
				// change the name of the slide to add "(1/$nbSlides)"...
				char tempChar[2];
				sprintf(tempChar, "%d", nbSlides);
				
				// ... but we don't want to add a supplementary line
				float after = 0;
				
				std::string maxTextAdded;
				maxTextAdded = " (";
				maxTextAdded += tempChar;
				maxTextAdded += "/";
				maxTextAdded += tempChar;
				maxTextAdded += ")";

				while( sizeBlockTitle != after )
				{
					// first iteration after==0 -> do nothing
					// others iterations with remove the last character of text until the number of lines is the same than before 
					if( after != 0 )
					{
						text.pop_back();
					}
					
					std::string tempText = text;
					tempText += maxTextAdded;
					
					after = countSize(tempText,2);	
				}
				
				text += " (1/";
				text += tempChar;
				text += ")";
				
				v.second.put<std::string>("title",text);
			}
			
			// if we are going to cut the slide we don't want to fuse with the next one
			if( nbSlides > 1 && m_modeFuse==1 )
			{
				tabCpt.push_back( spaceAvailable );
			}
			else
			{
				tabCpt.push_back( (cpt+sizeBlockTitle) );
			}
			
			tabSlides.push_back( nbSlides );
		}
	}
	
	if( m_modeFuse > 0 )
	{
		// std::cout << "name= "<< _pt.get< std::string >("title");
		// for(int i=0;i<tabCpt.size();++i)
		// {
			// std::cout << " " << tabCpt[i];
		// }
		// std::cout << std::endl;
		// std::cout << "sp= " << spaceAvailable << std::endl;
		
		float sum = tabCpt[0];
		std::vector< int > tabState;
		tabState.resize(tabCpt.size(),0);
		// we want to fuse slides with few elements only
		// three states for fuse: begin (1), continue (3) and end (2)
		for(int i=1;i<tabCpt.size();++i)
		{
			if( ((sum+tabCpt[i]) < spaceAvailable) && (tabSlides[i]==1) )
			{
				++tabState[i-1];
				tabState[i]+=2;
				
				sum += tabCpt[i];
			}
			else
			{
				sum = tabCpt[i];	
			}
			
			if(tabCpt[i] > spaceAvailable )
			{
				std::cout << "we have already manage the \"too big\" slides but n="<< i << std::endl;
			}
		}
		
		// add new node to indicate where to put the LaTeX tags
		int cpt=0;
		BOOST_FOREACH(ptree::value_type &v, _pt.get_child("children.topics"))// a slide
		{
			if( v.first == "topic" )
			{
				// fuse slides with new nodes 
				if( tabState[cpt] == 1 )
				{
					v.second.put_child("fuseBegin", ptree() );
				}
				
				if( tabState[cpt] == 3 )
				{
					v.second.put_child("fuseContinue", ptree() );
				}
				
				if( tabState[cpt] == 2 )
				{
					v.second.put_child("fuseEnd", ptree() );
				}
				
				++cpt;
			}
		}
	}
}

// check if we can create a subsection or not ie all the children of its children have at least one child
float checkSplitIntoSubsection( boost::property_tree::ptree _pt )
{
	float cptSlideWithoutChild=0.0;
	float cptChild=0.0;
	BOOST_FOREACH(boost::property_tree::ptree::value_type &v, _pt.get_child("children.topics"))
	{// check for all the future subsections
		if( v.first == "topic" )
		{				
			if( v.second.count("children") != 0)
			{						
				BOOST_FOREACH(boost::property_tree::ptree::value_type &v3, v.second.get_child("children.topics"))
				{// check for all the slides in the subsections
					if( (v3.first == "topic") && (v3.second.count("children") == 0) )
					{// if they don't have child
						++cptSlideWithoutChild;
					}
					
					if( (v3.first == "topic") )
					{
						++cptChild;
					}
				}
			}
			else
			{
				++cptSlideWithoutChild;
			}
			
			++cptChild;
		}
	}
	
	return (cptSlideWithoutChild/cptChild);
}

void visitChildSlide( boost::property_tree::ptree _pt, std::ofstream& _latexFile , int _depthInsideSlide, std::vector < std::string >& _nameParent)
{
	std::string text;
	std::string marker="";
		
	std::string whitespace;
	for(int i=0;i<_depthInsideSlide;++i)
	{
		whitespace+="  ";
	}
	
	// if we have to cut just before the first child it will create an empty 
	bool firstChildIsCut=false;
	if( _pt.get_child("children.topics.topic").count("cut") != 0 )
	{
		firstChildIsCut = true;
	}
	
	// we cannot have more than 4-depth native latex list... 
	// we can use enumitem package but we lose the magic [<+->] option...
	// and a 4-depth list on one slide seems crappy so we don't allow that
	if( _depthInsideSlide < 4 )
	{
		if( !firstChildIsCut )// we don't want to open a new itemize as we are going to "close" the slide
		{
			_latexFile<<whitespace<<"\\begin{itemize}[<+->]"<<std::endl;
		}
	}

	BOOST_FOREACH(boost::property_tree::ptree::value_type &v, _pt.get_child("children.topics")) 
	{
		if( v.first == "topic" )
		{		
			text = v.second.get<std::string>("title");
			
			if( v.second.count("cut") != 0 )// cut the slide (close/open frame/block/itemize tags)
			{
				std::cout << text << std::endl;
				++m_numSubSlide;
				cutSlide( _latexFile,_depthInsideSlide,_nameParent,firstChildIsCut);
			}
			
			if( _depthInsideSlide < 4 )
			{
				_nameParent[_depthInsideSlide] = text;
			}
			
			if( v.second.count("marker-refs") != 0 )// this one have a marker
			{		
				// take the first one only
				marker = v.second.get< std::string >("marker-refs.marker-ref.<xmlattr>.marker-id");
				
				// replace the '-' by a '_'
				while( marker.find('-') != std::string::npos )
				{
					marker.replace(marker.find("-"),1,"_");
				}
				
				// add a @32 (not the other markers)
				if( marker.find("other") == std::string::npos )
				{
					marker += "@32";
				}
				
				if( marker.find("task") != std::string::npos )
				{
					std::string newMarker = "progress";
					newMarker += marker.substr(4,marker.size()-4);
					marker = newMarker;
					
				}
				
				marker += ".png";
			}
		
			if( text != "" )// for example, for an image without legend
			{				
				if( marker != "" )
				{
					_latexFile<<whitespace<<"  \\item[{\\includegraphics[height=0.5\\baselineskip]{../markers/" << marker << "}}] ";
					
					marker = "";
				}
				else
				{
					_latexFile<<whitespace<<"  \\item ";
				}
				addTextInsideSlide( v.second,_latexFile,text, _depthInsideSlide);
				_latexFile<<std::endl; 
				
			}
			
			if( v.second.count("xhtml:img") != 0 )// this one have an image
			{
				if( text == "" ) // no text so possibly no /item tag
				{
					_latexFile <<whitespace<<"  \\item[]" << std::endl;
				}
				text = v.second.get<std::string>("xhtml:img.<xmlattr>.xhtml:src");
				// this string is "xap:filepathimg" so we have to remove the xap:
				text.replace(0,4,"");
				
				// if the image is a .jpg one, we must have generate a transparent background .png file
				if( text.find(".jpg") != std::string::npos )
				{
					text.replace(text.find(".jpg"),4,".png");
				}

				int width  = v.second.get< int >("xhtml:img.<xmlattr>.svg:width");
				int height = v.second.get< int >("xhtml:img.<xmlattr>.svg:height");
			
				_latexFile <<whitespace<<"  \\begin{figure}" << std::endl;
				_latexFile <<whitespace<<"    \\centerline{\\includegraphics[";
				_latexFile << "height=" << height << "pt,";
				_latexFile << "width=" << width << "pt]{";
				_latexFile << text << "}}" << std::endl;
				_latexFile <<whitespace<<"    \\caption{}" << std::endl;
				_latexFile <<whitespace<<"  \\end{figure}" << std::endl;
				
			}			

			if( v.second.count("children") != 0)
			{		
				visitChildSlide( v.second, _latexFile, _depthInsideSlide+1, _nameParent);// sentence or image
			}
		}
	}	
	
	// same as before 
	if( _depthInsideSlide < 4 )
	{
		_latexFile<<whitespace<<"\\end{itemize}"<<std::endl;
	}
}

void addSlideWithTitleOnly( std::ofstream& _latexFile, std::string _text )
{
	_latexFile <<"\\begin{beamercolorbox}[wd=.94\\paperwidth,left,rounded=true]{block title}" << std::endl;
	addText( _latexFile, _text );
	_latexFile<<"\\end{beamercolorbox}"<<std::endl;
}

void addLastSlide( std::ofstream& _latexFile )
{
	// add the last slide
	_latexFile << "\\begin{frame}" << std::endl;
	_latexFile << "\\onslide<+->" << std::endl;
	_latexFile << "\\begin{beamercolorbox}[wd=.22\\paperwidth,left,rounded=true,shadow=true]{block title}" << std::endl;
	_latexFile << "   \\Huge{It was...}" << std::endl;
	_latexFile << "\\end{beamercolorbox}" << std::endl;
	_latexFile << "\\onslide<+->" << std::endl;
	_latexFile << "\\titlepage" << std::endl;
	_latexFile << "\\onslide<+->" << std::endl;
	_latexFile << "\\hfill \\begin{beamercolorbox}[wd=.79\\paperwidth,left,rounded=true,shadow=true]{block title}" << std::endl;
	_latexFile << "   \\Huge{Thank you for your attention!}" << std::endl;
	_latexFile << "\\end{beamercolorbox}" << std::endl;
	_latexFile << "\\end{frame}" << std::endl;
	_latexFile << std::endl;	
}

void visitGraph( boost::property_tree::ptree _pt, std::ofstream& _latexFile , int _depth, bool& _appendix)
{
	using boost::property_tree::ptree;
	
	std::string text; 
	
	BOOST_FOREACH(ptree::value_type &v, _pt.get_child("children.topics"))
	{
		if( v.first == "topic" )
		{
			text = v.second.get<std::string>("title");
			
			if( _depth == 1 )// section
			{
				if( text == m_keywordAppendix )// special separator for the supplementary (hidden) slides
				{
					addLastSlide( _latexFile );
					
					_latexFile << "\\appendix" << std::endl;
					_latexFile << std::endl;
					_appendix = true;
				}
				else
				{
					_latexFile <<"\\section{";
					m_sizeFrameTitle = countSize( text, 1 );
					addText( _latexFile,text);
					_latexFile<<"}"<<std::endl;
					_latexFile <<std::endl;	
					
					// check if the section have child
					if( v.second.count("children") == 0)
					{// if no child, just create a single slide with the section title...
						_latexFile <<"\\begin{frame}{\\insertsectionhead:}"<<std::endl;
						addSlideWithTitleOnly( _latexFile, text );
						_latexFile <<"\\end{frame}"<<std::endl;
						_latexFile <<std::endl;
					}
					else
					{
						// check if we can create a subsection or not (we want to be sure that at least 20% of the slides in all the subsections have at least one element)
						if( checkSplitIntoSubsection(v.second) <= m_thresholdSubsection )
						{
							visitGraph(v.second,_latexFile,2,_appendix);// do a subsection
						}
						else
						{
							// before creating slides, check if we can fuse some slides according to their number of elements
							opFuseOrCutSlide( v.second );

							visitGraph(v.second,_latexFile,3,_appendix);// skip the subsection and do slides
						}	
					}	
				}					
			}
			
			if( _depth == 2 )// subsection
			{	
				std::string subsection = text;
				_latexFile <<"\\subsection{";
				addText( _latexFile, subsection);
				_latexFile<<"}"<<std::endl;
				_latexFile <<std::endl;
				
				// add the subsection name to each child title
				std::string textTemp;
				BOOST_FOREACH(ptree::value_type &v2, v.second.get_child("children.topics"))
				{	
					if( v2.first == "topic" )
					{
						textTemp = text;
						textTemp += ": ";
						textTemp += v2.second.get<std::string>("title");
						
						v2.second.put<std::string>("title",textTemp);
					}
				}
			
				// before creating slides, check if we can fuse some slides according to their number of elements
				opFuseOrCutSlide( v.second );
				
				visitGraph(v.second,_latexFile,3,_appendix);
			}
			
			if( _depth == 3 )// slide
			{			
				if( (v.second.count("fuseContinue") == 0) && (v.second.count("fuseEnd") == 0) )
				{
					_latexFile <<"\\begin{frame}{\\insertsectionhead:}"<<std::endl;
				}
				else
				{
					_latexFile <<"\\vfill"<<std::endl;
				}

				_latexFile <<"\\onslide<+->"<<std::endl;
				
				if( v.second.count("children") != 0)
				{	
					_latexFile <<"\\begin{block}{";
					addText( _latexFile, text );
					_latexFile<<"}"<<std::endl;
			
					m_numSubSlide = 1;
					// visit each element of the slide
					std::vector < std::string > nameParent;
					nameParent.resize(4,"");
					nameParent[0] = text;
					visitChildSlide( v.second, _latexFile, 1, nameParent);	

					_latexFile <<"\\end{block}"<<std::endl;
				}
				else
				{
					addSlideWithTitleOnly( _latexFile, text );
				}
				
				if( (v.second.count("fuseContinue") == 0) && (v.second.count("fuseBegin") == 0) )
				{
					_latexFile <<"\\end{frame}"<<std::endl;
				}
				_latexFile <<std::endl;
			}
		}
	}
}

void main( int argc, char** argv)
{	
	using boost::property_tree::ptree;

	std::string folder = argv[1];
	
	if( argc == 3)
	{
		m_modeFuse = std::atoi(argv[2]);
	}
	
	std::ifstream stylesFile;
	stylesFile.open(folder+"/styles.xml");// input: styles.xml

	// the styles.xml is not always generated (when no specific style was created during .xmind edition)
	if( stylesFile.is_open() )
	{
		// parse the styles.xml to retrieve all the styles defined in the .xmind file
		ptree ptStyle;
		read_xml(stylesFile, ptStyle, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);
		retrieveStyles( ptStyle );
	}

	// read content.xml and create the graph
	std::ifstream contentFile;
	contentFile.open(folder+"/content.xml");// input: content.xml
	
    ptree pt;
    read_xml(contentFile, pt, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);
	
	std::ofstream latexFile;
	latexFile.open(folder+"/Temp.tex");// output: latex file
	
	std::string text;
	
	fillSizeTable();
	
	pt = pt.get_child("xmap-content.sheet");
	text = pt.get<std::string>("topic.title");// the title of the presentation (main subject)
	
	// add the header and the beginning of the latex file
	latexFile << "\\input{../Header}" << std::endl;
	latexFile << std::endl;
	latexFile << "\\title{";
	addText(latexFile, text);
	latexFile << "}"<<std::endl;
	latexFile << std::endl;
	latexFile << "\\begin{document}" << std::endl;
	latexFile << std::endl;
	latexFile << "\\begin{frame}[plain]" << std::endl;
	latexFile << "\\titlepage" << std::endl;
	latexFile << "\\end{frame}" << std::endl;
	latexFile << std::endl;	
	
	// add a outline slide
	latexFile << "\\begin{frame}[c]{Outline}" <<std::endl;
	latexFile << "\\label{outline}" <<std::endl;
	latexFile << "\\begin{multicols}{2}" <<std::endl;
	latexFile << "   \\tableofcontents[" <<std::endl;
	latexFile << "       sectionstyle=show/show," <<std::endl;
	latexFile << "       subsectionstyle=hide/hide/hide]" <<std::endl;
	latexFile << "\\end{multicols}" <<std::endl;
	latexFile << "\\end{frame}" <<std::endl;
	latexFile <<std::endl;
	
	// recursive function
	bool appendix=false;
	visitGraph( pt.get_child("topic"), latexFile, 1, appendix );
	
	// add the last slide if there wasn't an appendix section
	if( !appendix )
	{
		addLastSlide( latexFile );
	}

	latexFile <<"\\end{document}"<<std::endl;
}



























