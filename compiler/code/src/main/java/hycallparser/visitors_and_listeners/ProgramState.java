package hycallparser.visitors_and_listeners;

import hycallparser.antlr.HycallParser;
import hycallparser.builtinProcedures.ProcedureManager;
import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;

import java.math.BigInteger;
import java.util.HashMap;
import java.util.Optional;

public class ProgramState {

    private HashMap<String, Parameter> globalVariables;
    private HashMap<String, HycallParser.ProcedureContext> procedures;

    public ProgramState() {
        globalVariables = new HashMap<>();
        procedures = new HashMap<>();
    }

    public void addNewGlobalVar(String name) {
        if (existGlobalVar(name))
            throw new IllegalArgumentException("Global variable \"" + name + "\" is declared twice!");
        globalVariables.put(name, new ParameterInteger(BigInteger.valueOf(0)));
    }

    public void addNewGlobalVar(String name, BigInteger value) {
        if (existGlobalVar(name))
            throw new IllegalArgumentException("Global variable \"" + name + "\" is declared twice!");
        globalVariables.put(name, new ParameterInteger(value));
    }

    public boolean existGlobalVar(String name) {
        return globalVariables.containsKey(name);
    }

    public Parameter getGlobalVar(String name) {
        return Optional.ofNullable(globalVariables.get(name)).orElseThrow(() -> new IllegalArgumentException("Tried accessing global variable with name \"" + name + "\", but does not exist!"));
    }

    public void setGlobalVariable(String name, Parameter value) {
        if (!existGlobalVar(name))
            throw new IllegalArgumentException("Tried setting global variable \"" + name + "\", but does not exist!");
        globalVariables.put(name, value);
    }

    public void addProcedure(HycallParser.ProcedureContext pc) {
        String name = pc.IDENTIFIER().getText();
        if (ProcedureManager.existsBuiltinWithName(name))
            throw new IllegalArgumentException("Illegal procedure name \"" + name + "\", there is already a builtin procedure with this name!");
        if (procedures.containsKey(name))
            throw new IllegalArgumentException("Procedure \"" + name + "\" is declared twice!");
        procedures.put(name, pc);
    }

    public boolean existsProcedure(String name) {
        return procedures.containsKey(name);
    }

    public HycallParser.ProcedureContext getProcedure(String name) {
        return Optional.ofNullable(procedures.get(name)).orElseThrow(() -> new IllegalArgumentException("Procedure \"" + name + "\" does not exist!"));
    }
}