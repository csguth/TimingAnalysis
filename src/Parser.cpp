#include "include/Parser.h"

Parser::Parser()
{

}

Parser::~Parser()
{

}

bool Parser::isSpecialChar(const char & c)
{
	static const char specialChars[] =
	{ '(', ')', ',', ':', ';', '/', '#', '[', ']', '{', '}', '*', '\"', '\\' };

	for (unsigned i = 0; i < sizeof(specialChars); ++i)
	{
		if (c == specialChars[i])
			return true;
	}

	return false;	
}


bool Parser::readLineAsTokens(istream& is, vector<string>& tokens, bool includeSpecialChars)
{
	tokens.clear() ;

	string line ;
	std::getline (is, line) ;

	while (is && tokens.empty()) {

		string token = "" ;

		for (int i=0; i < line.size(); ++i) {
			char currChar = line[i] ;
			bool _isSpecialChar = isSpecialChar(currChar) ;

			if (std::isspace (currChar) || _isSpecialChar) {

			if (!token.empty()) {
				// Add the current token to the list of tokens
				tokens.push_back(token) ;
				token.clear() ;
			}

			if (includeSpecialChars && _isSpecialChar) {
				tokens.push_back(string(1, currChar)) ;
			}

			} else {
				// Add the char to the current token
				token.push_back(currChar) ;
			}

		}

		if (!token.empty())
			tokens.push_back(token) ;


		if (tokens.empty())
			// Previous line read was empty. Read the next one.
			std::getline (is, line) ;
	}

	//for (int i=0; i < tokens.size(); ++i)
	//  cout << tokens[i] << " " ;
	//cout << endl ;

	return !tokens.empty() ;
}


// VERILOG PARSER
bool VerilogParser::readFile(const string filename, CircuitNetList & netlist)
{

}


// CIRCUIT NETLIST
const int CircuitNetList::addGate(const string name, const string cellType, const int inputs)
{
	if(gateNameToGateIndex.find(name) != gateNameToGateIndex.end())
		return gateNameToGateIndex[name];

	//const string name, const string cellType, const unsigned inputs, int fanoutNetIndex
	gates.push_back(LogicGate(name, cellType, inputs, -1));
	gateNameToGateIndex[name] = gates.size() - 1;
	return gateNameToGateIndex[name];
}

const int CircuitNetList::addNet(const string name)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
		return netNameToNetIndex[name];

	nets.push_back(Net(name, -1, "o"));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

const int CircuitNetList::addNet(const string name, const int sourceNode, const string sourcePin)
{
	if(netNameToNetIndex.find(name) != netNameToNetIndex.end())
		return netNameToNetIndex[name];

	nets.push_back(Net(name, sourceNode, sourcePin));
	netNameToNetIndex[name] = nets.size() - 1;
	return netNameToNetIndex[name];
}

void CircuitNetList::addCellInst(const string name, const string cellType, vector<pair<string, string> > inputPinPairs)
{
	const string outputPin = inputPinPairs.back().first;
	const string fanoutNetName = inputPinPairs.back().second;
	const int gateIndex = addGate(name, cellType, inputPinPairs.size() - 1);
	const int netIndex = addNet(fanoutNetName, gateIndex, outputPin);
	LogicGate & gate = gates[gateIndex];
	gate.fanoutNetIndex = netIndex;

	for(size_t i = 0; i < inputPinPairs.size() - 1 ; i++)
	{
		const int faninNetIndex = addNet(inputPinPairs[i].second);
		Net & faninNet = nets[faninNetIndex];
		faninNet.addSink(Sink(gateIndex, inputPinPairs[i].first));
		gate.inNets[i] = faninNetIndex;
	}
	

}

