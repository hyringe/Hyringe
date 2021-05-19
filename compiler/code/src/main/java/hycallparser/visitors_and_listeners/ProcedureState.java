package hycallparser.visitors_and_listeners;

import hycallparser.parameter.Parameter;

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;

public class ProcedureState {

    private HashMap<String, Parameter> variables;

    public ProcedureState(Map<String, Parameter> parameters) {
        this.variables = new HashMap<>(parameters);
    }

    public ProcedureState() {
        variables = new HashMap<>();
    }

    public boolean existVar(String name) {
        return variables.containsKey(name);
    }

    public Parameter getVar(String name) {
        return Optional.ofNullable(variables.get(name)).orElseThrow(() -> new IllegalArgumentException("Tried accessing local variable with name \"" + name + "\", but does not exist!"));
    }

    public void setVariable(String name, Parameter value) {
        variables.put(name, value);
    }
}
